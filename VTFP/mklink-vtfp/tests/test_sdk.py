"""Tests for the VTFPAgent SDK using a mock bridge."""

import struct
import pytest
from unittest.mock import MagicMock

from mklink_vtfp.sdk import (
    VTFPAgent,
    VTFPAgentError,
    VTFPAgentNotFoundError,
    VTFPAgentVersionError,
    VTFPAgentTimeoutError,
    VTFPAgentSafetyError,
    VTFPAgentUnknownCmdError,
    QueryInfoResult,
    CommandResult,
    load_project_config,
)
from vtfp_proto import VTFP_MAGIC, VTFP_VERSION, R_OK, R_NOT_ARMED


SAMPLE_YAML_DICT = {
    "project": {"name": "test", "product_id": "TEST-1.0"},
    "protocol": {"version": "0.1.0"},
    "memory": {"header_base": "0x20000000", "data_buffer_size": 1024},
    "commands": [
        {"id": 1, "name": "TOGGLE_OUTPUT", "direction": "pc_to_mcu", "requires_arm": True},
        {"id": 2, "name": "READ_SENSOR", "direction": "pc_to_mcu", "requires_arm": False},
    ],
}


class MockBridge:
    """A minimal mock of MKLinkSerialBridge that simulates a 44-byte shared header."""

    def __init__(self):
        self.ram = bytearray(64 + 1024)  # mock shared RAM
        # Initialize the header in the mock RAM
        self._write_header_magic_version()
        # Command poll: result starts as 0xFF (no result yet)
        self._timeout_count = 0

    def _write_header_magic_version(self):
        """Write magic + version + features at the start of the header."""
        struct.pack_into("<I", self.ram, 0, VTFP_MAGIC)
        struct.pack_into("<H", self.ram, 4, VTFP_VERSION)
        # features = 0 (default)
        # data_addr at offset 24
        struct.pack_into("<I", self.ram, 24, 0x20000000 + 64)
        # data_len at offset 28 = 0
        # checksum at offset 36 = 0
        # Compute and write the correct checksum over bytes 0..36
        from vtfp_proto import crc32
        c = crc32(bytes(self.ram[0:36]))
        struct.pack_into("<I", self.ram, 36, c)

    def _read_u32(self, addr):
        return struct.unpack_from("<I", self.ram, addr - 0x20000000)[0]

    def _write_u32(self, addr, val):
        struct.pack_into("<I", self.ram, addr - 0x20000000, val & 0xFFFFFFFF)

    def _read_bytes(self, addr, n):
        return bytes(self.ram[addr - 0x20000000:addr - 0x20000000 + n])

    def _write_bytes(self, addr, data):
        self.ram[addr - 0x20000000:addr - 0x20000000 + len(data)] = data

    def send_command(self, cmd, timeout=None):
        """Simulate a text command. For our tests, just return a hex response."""
        # For cmd.read_ram(0xADDR, 4): return 4 bytes as hex
        if "read_ram" in cmd:
            # Parse the address
            import re
            m = re.search(r"0x([0-9A-Fa-f]+),\s*(\d+)", cmd)
            if m:
                addr = int(m.group(1), 16)
                n = int(m.group(2))
                data = bytes(self.ram[addr - 0x20000000:addr - 0x20000000 + n])
                return data.hex().upper()
        if "write_ram" in cmd:
            # Parse addr, then bytes
            import re
            m = re.search(r"0x([0-9A-Fa-f]+),\s*(.*)", cmd)
            if m:
                addr = int(m.group(1), 16)
                rest = m.group(2)
                # Parse bytes like 0x01, 0x02, 0x03
                byte_vals = re.findall(r"0x([0-9A-Fa-f]{2})", rest)
                for i, bv in enumerate(byte_vals):
                    self.ram[addr - 0x20000000 + i] = int(bv, 16)
                # Simulate MCU processing of the command: if the command is at offset 8,
                # the MCU would clear command and set result.
                if addr == 0x20000008:
                    # After SDK writes the command, MCU processes it
                    # Clear command at offset 8
                    self._write_u32(0x20000008, 0)
                    # Set result at offset 12 to R_OK
                    self._write_u32(0x2000000C, 0)
        return ""

    def close(self):
        pass


def make_agent():
    bridge = MockBridge()
    return VTFPAgent(bridge=bridge, config=SAMPLE_YAML_DICT, header_base=0x20000000)


class TestVTFPAgentDiscovery:
    def test_discover_finds_mcu(self):
        a = make_agent()
        info = a.discover()
        # info.product_id will be "unknown" because QUERY_INFO returns
        # empty in this minimal mock (no handler ran)
        assert info.protocol_version == VTFP_VERSION

    def test_discover_raises_on_wrong_magic(self):
        a = make_agent()
        # Corrupt the magic
        a.bridge._write_u32(0x20000000, 0xDEADBEEF)
        with pytest.raises(VTFPAgentNotFoundError, match="magic"):
            a.discover()


class TestVTFPAgentInvoke:
    def test_invoke_unknown_command_raises(self):
        a = make_agent()
        with pytest.raises(VTFPAgentUnknownCmdError, match="Unknown command"):
            a.invoke("NONEXISTENT_CMD")

    def test_invoke_dangerous_without_arm_raises(self):
        a = make_agent()
        # Don't auto-arm
        with pytest.raises(VTFPAgentSafetyError, match="requires ARM"):
            a.invoke("TOGGLE_OUTPUT", auto_arm=False)


class TestProjectConfigLoader:
    def test_load_returns_dict(self, tmp_path):
        # Write a minimal vtfp.yaml
        yaml_path = tmp_path / "vtfp.yaml"
        yaml_path.write_text("""
project:
  name: tmp
  product_id: TMP-1.0
protocol:
  version: "0.1.0"
memory:
  header_base: "0x20000000"
  data_buffer_size: 1024
commands:
  - id: 1
    name: X
    direction: pc_to_mcu
    requires_arm: false
""", encoding="utf-8")
        cfg = load_project_config(str(yaml_path))
        assert cfg["project"]["name"] == "tmp"
        assert len(cfg["commands"]) == 1


class TestResultClasses:
    def test_command_result_ok_property(self):
        r = CommandResult(result_code=R_OK)
        assert r.ok is True
        r2 = CommandResult(result_code=R_NOT_ARMED)
        assert r2.ok is False

    def test_query_info_result(self):
        q = QueryInfoResult(product_id="X", fw_version="0.1.0", protocol_version=1, features=0)
        assert q.product_id == "X"
        assert q.features == 0
