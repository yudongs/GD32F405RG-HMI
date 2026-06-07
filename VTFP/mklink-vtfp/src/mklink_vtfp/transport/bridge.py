"""MKLink serial bridge — wraps pyserial for MKLink read/write commands.

The MKLink CLI exposes a text protocol on the serial port. PC sends
`cmd.read_ram(addr, n)` / `cmd.write_ram(addr, bytes)` / `cmd.dump_memory(...)`
as text lines; MCU responds with hex output.

This class provides a context-managed serial connection with thread-safe
read/write and a stream-mode for dump_memory's high-throughput mode.
"""

import time
import threading
import serial
from typing import Optional


class PortBusyError(Exception):
    """Raised when the COM port is held by another process."""
    def __init__(self, message: str, holder_pid: Optional[int] = None):
        super().__init__(message)
        self.holder_pid = holder_pid


class MKLinkSerialBridge:
    """Context-managed MKLink serial connection."""

    def __init__(self, port: str, baudrate: int = 115200, exclusive: bool = True, timeout: float = 5.0):
        self.port = port
        self.baudrate = baudrate
        self.exclusive = exclusive
        self.timeout = timeout
        self._ser: Optional[serial.Serial] = None
        self._lock = threading.Lock()
        self._stream_mode = False

    def connect(self) -> bool:
        """Establish the serial connection."""
        with self._lock:
            if self._ser is not None:
                return True
            try:
                self._ser = serial.Serial(
                    port=self.port,
                    baudrate=self.baudrate,
                    timeout=self.timeout,
                    exclusive=self.exclusive,
                )
            except Exception as e:
                # Try to extract a "PID" hint from the error message
                # (Windows format: "PermissionError(13, 'Access is denied')")
                raise PortBusyError(f"Cannot open {self.port}: {e}") from e
            return True

    def close(self) -> None:
        """Close the connection (idempotent)."""
        with self._lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                except Exception:
                    pass
                self._ser = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc, tb):
        self.close()
        return False

    def send_command(self, cmd: str, timeout: Optional[float] = None) -> str:
        """Send a text command and read the response (line-based).

        Returns the response text (with trailing newline stripped).
        """
        if self._ser is None:
            raise RuntimeError("Bridge not connected")
        with self._lock:
            self._ser.reset_input_buffer()
            self._ser.write((cmd + "\n").encode("utf-8"))
            self._ser.flush()
            # Read until we get a complete line or timeout
            to = timeout if timeout is not None else self.timeout
            end = time.monotonic() + to
            buf = b""
            while time.monotonic() < end:
                if self._ser.in_waiting > 0:
                    chunk = self._ser.read(self._ser.in_waiting)
                    buf += chunk
                    if b"\n" in buf:
                        break
                else:
                    time.sleep(0.01)
            return buf.decode("utf-8", errors="replace").strip()

    def read_bytes(self, n: int, timeout: Optional[float] = None) -> bytes:
        """Read exactly n bytes (with timeout)."""
        if self._ser is None:
            raise RuntimeError("Bridge not connected")
        to = timeout if timeout is not None else self.timeout
        end = time.monotonic() + to
        buf = bytearray()
        while len(buf) < n and time.monotonic() < end:
            chunk = self._ser.read(n - len(buf))
            if chunk:
                buf.extend(chunk)
        return bytes(buf)

    def write_bytes(self, data: bytes) -> None:
        """Write raw bytes."""
        if self._ser is None:
            raise RuntimeError("Bridge not connected")
        with self._lock:
            self._ser.write(data)
            self._ser.flush()
