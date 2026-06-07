"""High-level VTFP agent SDK.

Wraps the transport layer (MKLink serial bridge) and provides a Pythonic
API for AI/MCP clients to control the embedded MCU.

Typical usage:
    from mklink_vtfp import VTFPAgent, load_project_config

    config = load_project_config("vtfp.yaml")
    agent = VTFPAgent(port="COM5", config=config)
    agent.discover()

    info = agent.query_info()
    print(f"Connected to {info.product_id} v{info.fw_version}")

    # User-defined command
    result = agent.invoke("TOGGLE_OUTPUT", param=0x01)

    # Dangerous command (requires ARM)
    agent.arm()
    result = agent.invoke("SET_MODE", param=0x02)
    agent.disarm()
"""

from __future__ import annotations

import os
import time
import yaml
from dataclasses import dataclass, field
from typing import Any, Optional, Dict, List

from vtfp_proto import (
    VTFP_MAGIC, VTFP_VERSION, SAFETY_KEY, ARM_TIMEOUT_MS,
    pack_header, unpack_header, crc32,
    R_OK, R_NOT_ARMED, R_ARM_EXPIRED, R_SAFETY_KEY, R_PARAM_RANGE,
    R_NOT_INIT, R_BUF_OVERFLOW, R_UNKNOWN_CMD,
    CMD_ARM, CMD_DISARM, CMD_QUERY_INFO, CMD_QUERY_STATE,
)
from vtfp_proto.header import VTFHeader
from vtfp_proto.constants import (
    CMD_USER_BASE, CMD_USER_MAX, CMD_STD_BASE, CMD_STD_MAX,
)

from .transport import MKLinkSerialBridge, get_default_port, get_default_baudrate


# --- Exceptions ---

class VTFPAgentError(Exception):
    """Base exception for VTFPAgent."""


class VTFPAgentNotFoundError(VTFPAgentError):
    """MCU not responding (no magic at expected address)."""


class VTFPAgentVersionError(VTFPAgentError):
    """Protocol version mismatch."""


class VTFPAgentCorruptError(VTFPAgentError):
    """Header checksum failed."""


class VTFPAgentTimeoutError(VTFPAgentError):
    """Poll timeout (MCU did not respond in time)."""


class VTFPAgentSafetyError(VTFPAgentError):
    """ARM-related failure (NOT_ARMED, ARM_EXPIRED, SAFETY_KEY, etc.)."""


class VTFPAgentDataError(VTFPAgentError):
    """Data segment overflow or corruption."""


class VTFPAgentUnknownCmdError(VTFPAgentError):
    """Unknown command (not in dispatch table)."""


# --- Result classes ---

@dataclass
class QueryInfoResult:
    product_id: str
    fw_version: str
    protocol_version: int
    features: int
    raw: bytes = b""


@dataclass
class CommandResult:
    """Result of an `invoke()` call."""
    result_code: int
    data: bytes = b""
    elapsed_ms: float = 0.0

    @property
    def ok(self) -> bool:
        return self.result_code == R_OK


# --- Result code → exception mapping ---

_RESULT_EXCEPTIONS = {
    R_NOT_ARMED:     VTFPAgentSafetyError,
    R_ARM_EXPIRED:   VTFPAgentSafetyError,
    R_SAFETY_KEY:    VTFPAgentSafetyError,
    R_PARAM_RANGE:   VTFPAgentDataError,
    R_NOT_INIT:      VTFPAgentDataError,
    R_BUF_OVERFLOW:  VTFPAgentDataError,
    R_UNKNOWN_CMD:   VTFPAgentUnknownCmdError,
}


# --- Project config loader ---

def load_project_config(path: str = "vtfp.yaml") -> dict:
    """Load a project-level vtfp.yaml configuration."""
    with open(path) as f:
        return yaml.safe_load(f)


# --- The agent ---

class VTFPAgent:
    """High-level agent for controlling an embedded MCU running VTFP v1."""

    def __init__(self, port: Optional[str] = None, baudrate: Optional[int] = None,
                 config: Optional[dict] = None, bridge: Optional[MKLinkSerialBridge] = None,
                 header_base: int = 0x20000000, data_addr: int = 0x20000000 + 44,
                 data_size: int = 1024, timeout: float = 2.0):
        """Create an agent. If `bridge` is provided, port/baudrate are ignored.

        `config` is a parsed vtfp.yaml dict (from load_project_config()).
        """
        self.port = port or get_default_port()
        self.baudrate = baudrate if baudrate is not None else get_default_baudrate()
        self.config = config or {}
        self.bridge = bridge if bridge is not None else MKLinkSerialBridge(self.port, self.baudrate)
        self.header_base = header_base
        self.data_addr = data_addr
        self.data_size = data_size
        self.timeout = timeout

        # Discovered state
        self._info: Optional[QueryInfoResult] = None
        self._commands: Dict[str, int] = {}  # name -> cmd_id (from config)
        self._requires_arm: Dict[int, bool] = {}  # cmd_id -> requires_arm

        # Parse commands from config
        for cmd in self.config.get("commands", []):
            name = cmd.get("name", "")
            cmd_id = cmd.get("id")
            if name and cmd_id is not None:
                self._commands[name] = cmd_id
                self._requires_arm[cmd_id] = cmd.get("requires_arm", False)

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, *args):
        self.close()

    def connect(self):
        self.bridge.connect()

    def close(self):
        self.bridge.close()

    # --- Low-level RAM access (used internally and exposed for power users) ---

    def _read_u32(self, addr: int) -> int:
        """Read 4 bytes from MCU SRAM via cmd.read_ram and return as little-endian uint32."""
        import struct
        text = self.bridge.send_command(f"cmd.read_ram(0x{addr:08X}, 4)")
        raw = bytes.fromhex(text.strip().replace(" ", ""))
        return struct.unpack_from("<I", raw, 0)[0]

    def _write_u32(self, addr: int, val: int):
        """Write 4 bytes to MCU SRAM."""
        b = val.to_bytes(4, "little")
        args = ", ".join(f"0x{byte:02X}" for byte in b)
        self.bridge.send_command(f"cmd.write_ram(0x{addr:08X}, {args})")

    def _read_bytes(self, addr: int, n: int) -> bytes:
        """Read n bytes from MCU SRAM."""
        text = self.bridge.send_command(f"cmd.read_ram(0x{addr:08X}, {n})")
        return bytes.fromhex(text.strip().replace(" ", ""))

    def _write_bytes(self, addr: int, data: bytes):
        """Write bytes to MCU SRAM (chunked)."""
        for i in range(0, len(data), 32):
            chunk = data[i:i+32]
            args = ", ".join(f"0x{b:02X}" for b in chunk)
            self.bridge.send_command(f"cmd.write_ram(0x{addr + i:08X}, {args})")

    # --- High-level operations ---

    def discover(self) -> QueryInfoResult:
        """Verify the MCU is alive and query its info. Returns QueryInfoResult.

        Raises VTFPAgentNotFoundError if magic is wrong.
        Raises VTFPAgentVersionError if protocol version doesn't match.
        Raises VTFPAgentCorruptError if checksum fails.
        """
        magic = self._read_u32(self.header_base)
        if magic != VTFP_MAGIC:
            raise VTFPAgentNotFoundError(
                f"VTFP magic not found at 0x{self.header_base:08X}: got 0x{magic:08X}, expected 0x{VTFP_MAGIC:08X}"
            )

        # Read the full 44-byte header
        hdr_bytes = self._read_bytes(self.header_base, 44)
        # Verify checksum
        expected_crc = crc32(hdr_bytes[0:36])
        import struct
        actual_crc = struct.unpack_from("<I", hdr_bytes, 36)[0]
        if actual_crc != expected_crc:
            raise VTFPAgentCorruptError(
                f"Header checksum mismatch: got 0x{actual_crc:08X}, computed 0x{expected_crc:08X}"
            )

        # Read version
        version = struct.unpack_from("<H", hdr_bytes, 4)[0]
        if version != VTFP_VERSION:
            raise VTFPAgentVersionError(
                f"Protocol version mismatch: got 0x{version:04X}, expected 0x{VTFP_VERSION:04X}"
            )

        # Read features
        features = struct.unpack_from("<H", hdr_bytes, 6)[0]
        # data_addr is at offset 24
        data_addr = struct.unpack_from("<I", hdr_bytes, 24)[0]
        if data_addr != self.data_addr:
            # Update to what MCU told us
            self.data_addr = data_addr

        # Now query INFO to get product_id and fw_version
        self._info = self.query_info()
        return self._info

    def query_info(self) -> QueryInfoResult:
        """Send QUERY_INFO (0x10) and parse the response.

        Response format (per vtfp_dispatch.c handle_query_info):
          offset 0: protocol_version (u16)
          offset 2: features (u16)
          offset 4: product_id_len (u16)
          offset 6: fw_version_len (u16)
          offset 8: product_id (NUL-terminated)
          offset 8+N: fw_version (NUL-terminated)
        """
        self._send_command_and_wait(CMD_QUERY_INFO, param=0)
        # Read the data segment
        hdr_bytes = self._read_bytes(self.header_base, 44)
        import struct
        data_len = struct.unpack_from("<I", hdr_bytes, 28)[0]
        if data_len == 0:
            # QUERY_INFO not implemented; return defaults
            return QueryInfoResult(
                product_id="unknown",
                fw_version="unknown",
                protocol_version=VTFP_VERSION,
                features=0,
            )
        data = self._read_bytes(self.data_addr, data_len)
        protocol_version = struct.unpack_from("<H", data, 0)[0]
        features = struct.unpack_from("<H", data, 2)[0]
        pid_len = struct.unpack_from("<H", data, 4)[0]
        fw_len = struct.unpack_from("<H", data, 6)[0]
        product_id = data[8:8+pid_len].rstrip(b"\x00").decode("utf-8", errors="replace")
        fw_version = data[8+pid_len:8+pid_len+fw_len].rstrip(b"\x00").decode("utf-8", errors="replace")
        return QueryInfoResult(
            product_id=product_id,
            fw_version=fw_version,
            protocol_version=protocol_version,
            features=features,
            raw=data,
        )

    def query_state(self) -> bytes:
        """Send QUERY_STATE (0x11) and return the 8-byte state word."""
        self._send_command_and_wait(CMD_QUERY_STATE, param=0)
        hdr_bytes = self._read_bytes(self.header_base, 44)
        import struct
        data_len = struct.unpack_from("<I", hdr_bytes, 28)[0]
        if data_len < 8:
            return b"\x00" * 8
        return self._read_bytes(self.data_addr, 8)

    def arm(self) -> None:
        """ARM the safety protocol. Must be called before invoking a
        dangerous command (one with requires_arm=true)."""
        self._send_command_and_wait(CMD_ARM, param=SAFETY_KEY, expect_ok=True)

    def disarm(self) -> None:
        """Disarm the safety protocol."""
        self._send_command_and_wait(CMD_DISARM, param=0, expect_ok=True)

    def invoke(self, cmd_name: str, param: int = 0, data: bytes = b"", auto_arm: bool = True) -> CommandResult:
        """Invoke a user-defined command by name.

        If the command requires ARM and auto_arm is True, automatically
        arm before invoking. If the command doesn't require ARM, the
        auto-arm is a no-op.

        Raises VTFPAgentSafetyError if ARM is required but auto_arm is False.
        """
        if cmd_name not in self._commands:
            raise VTFPAgentUnknownCmdError(
                f"Unknown command '{cmd_name}'. Known: {sorted(self._commands.keys())}"
            )
        cmd_id = self._commands[cmd_name]
        requires_arm = self._requires_arm.get(cmd_id, False)

        if requires_arm:
            if not auto_arm:
                raise VTFPAgentSafetyError(
                    f"Command '{cmd_name}' requires ARM, but auto_arm=False"
                )
            self.arm()

        # Write data segment first (if any)
        if data:
            if len(data) > self.data_size:
                raise VTFPAgentDataError(
                    f"Data too large: {len(data)} > {self.data_size}"
                )
            self._write_bytes(self.data_addr, data)
            # data_len will be written by _send_command_and_wait
        else:
            # Zero out data_len
            self._write_u32(self.header_base + 28, 0)  # data_len offset

        return self._send_command_and_wait(cmd_id, param=param)

    # --- Internal ---

    def _send_command_and_wait(self, cmd: int, param: int = 0, expect_ok: bool = False) -> CommandResult:
        """Write a command to the shared header and poll for the result.

        Sequence (per SPEC.md §2.2):
          1. Write seq (monotonic PC counter)
          2. Write param
          3. Write command
          4. Poll header.result with timeout
          5. Read data segment if data_len > 0
          6. Return CommandResult
        """
        if not hasattr(self, "_seq"):
            self._seq = 0
        self._seq += 1
        seq = self._seq

        # 1. Write seq (offset 16)
        self._write_u32(self.header_base + 16, seq)
        # 2. Write param (offset 20)
        self._write_u32(self.header_base + 20, param)
        # 3. Write command (offset 8) — this triggers MCU processing
        self._write_u32(self.header_base + 8, cmd)

        # 4. Poll result (offset 12) with timeout
        start = time.monotonic()
        deadline = start + self.timeout
        result_code = 0xFF
        while time.monotonic() < deadline:
            time.sleep(0.02)  # 20ms poll period
            try:
                result_code = self._read_u32(self.header_base + 12)
            except Exception:
                continue
            # MCU clears command to 0 after processing
            cmd_now = self._read_u32(self.header_base + 8)
            if cmd_now == 0 and result_code != 0xFF:
                break
        else:
            raise VTFPAgentTimeoutError(
                f"Command 0x{cmd:02X} timed out after {self.timeout}s"
            )

        elapsed = (time.monotonic() - start) * 1000

        # 5. Read data segment if data_len > 0
        data = b""
        try:
            import struct
            hdr_bytes = self._read_bytes(self.header_base, 44)
            data_len = struct.unpack_from("<I", hdr_bytes, 28)[0]
            if 0 < data_len <= self.data_size:
                data = self._read_bytes(self.data_addr, data_len)
        except Exception:
            pass

        # 6. Map result code to exception if not OK
        if result_code != R_OK:
            exc_class = _RESULT_EXCEPTIONS.get(result_code, VTFPAgentError)
            if expect_ok or result_code in (R_NOT_ARMED,):
                # Caller wanted OK; raise
                raise exc_class(
                    f"Command 0x{cmd:02X} failed: result={result_code}"
                )

        return CommandResult(result_code=result_code, data=data, elapsed_ms=elapsed)
