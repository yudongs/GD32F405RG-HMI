# LCD Framebuffer Web Viewer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a localhost web viewer that streams the MCU's `framebuffer` (240×160 @ 1bpp, 4800 bytes) to a browser at ~20 fps via WebSocket, with a `dump-memory` CLI subprocess as the data source.

**Architecture:** Three-layer (browser ← WebSocket → Python server ← subprocess stdout → mklink dump-memory). All Python code is async (`asyncio` + `websockets` lib). Frame parsing is pure-stdlib (`json`, `base64`). Image conversion uses `PIL`. Frontend is vanilla JS + canvas, no build step.

**Tech Stack:** Python 3.x, `websockets` 16.0, `asyncio`, `subprocess`, `PIL` 12.1, `pytest` 9.0; HTML5 canvas + WebSocket API.

**Spec:** `docs/superpowers/specs/2026-06-07-lcd-framebuffer-web-viewer-design.md`

---

## File Structure (created by this plan)

```
tools/lcd_viewer/
├── __init__.py            (empty)
├── config.py              (constants: FB_ADDR, FB_SIZE, DEFAULT_PERIOD_MS, PORT)
├── frame.py               (Frame dataclass + parse_dump_memory_json)
├── dumper.py              (Dumper class: subprocess mgmt + JSON line reader)
├── ws_server.py           (WebSocket + static file server, client count, broadcast)
├── server.py              (CLI entry: argparse + asyncio.run)
├── README.md              (usage + troubleshooting)
└── web/
    ├── index.html         (single page, no build)
    └── app.js             (WS client + canvas + controls)

tests/
├── __init__.py            (empty)
└── test_lcd_viewer/
    ├── __init__.py        (empty)
    ├── test_config.py
    ├── test_frame.py
    ├── test_dumper.py
    ├── test_ws_server.py
    └── test_integration.py
```

Total: 7 source files + 5 test files + 1 README.

---

## Phase 1: Foundation (Pure Logic, TDD)

### Task 1: Project skeleton + `config.py`

**Files:**
- Create: `tools/__init__.py`
- Create: `tools/lcd_viewer/__init__.py`
- Create: `tools/lcd_viewer/config.py`
- Create: `tests/__init__.py`
- Create: `tests/test_lcd_viewer/__init__.py`
- Create: `tests/test_lcd_viewer/test_config.py`

- [ ] **Step 1: Create directory tree**

```bash
mkdir -p tools/lcd_viewer/web tests/test_lcd_viewer
touch tools/__init__.py tools/lcd_viewer/__init__.py tests/__init__.py tests/test_lcd_viewer/__init__.py
```

- [ ] **Step 2: Write `tools/lcd_viewer/config.py`**

```python
"""Constants for the LCD framebuffer web viewer.

All paths and tunables live here. Don't import from anywhere else into
other modules of this package — keep this file dependency-free.
"""
from __future__ import annotations

# Framebuffer on the MCU (APP/lcd_st7586.c:18 — 240x160 @ 1bpp = 4800 bytes).
# Address resolved at runtime via `python -m mklink symbols` — see README.
FB_ADDR: int = 0x20001BA4
FB_SIZE: int = 4800          # 240 columns * 160 rows / 8 bits-per-byte
FB_WIDTH: int = 240
FB_HEIGHT: int = 160

# WebSocket / HTTP server.
HOST: str = "127.0.0.1"
PORT: int = 8765
WS_PATH: str = "/ws"

# Sampling period (ms). 50 ms = 20 fps. UI clamps to [10, 200].
DEFAULT_PERIOD_MS: int = 50
MIN_PERIOD_MS: int = 10
MAX_PERIOD_MS: int = 200

# WebSocket subprotocol (not used, but reserved for future versioning).
WS_SUBPROTOCOL: str | None = None

# Save-frame file prefix (browser downloads as <PREFIX>_<ts_us>.png).
SAVE_FILENAME_PREFIX: str = "frame"
```

- [ ] **Step 3: Write the failing test `tests/test_lcd_viewer/test_config.py`**

```python
"""Tests for tools.lcd_viewer.config — verifies the runtime constants."""
from tools.lcd_viewer import config


def test_fb_address_is_in_ram():
    # 0x20000000-0x20020000 is the GD32F405 SRAM region.
    assert 0x20000000 <= config.FB_ADDR < 0x20020000


def test_fb_size_matches_geometry():
    assert config.FB_SIZE == config.FB_WIDTH * config.FB_HEIGHT // 8


def test_default_period_within_bounds():
    assert config.MIN_PERIOD_MS <= config.DEFAULT_PERIOD_MS <= config.MAX_PERIOD_MS


def test_server_binds_to_localhost():
    assert config.HOST == "127.0.0.1"
    assert 1 <= config.PORT <= 65535


def test_ws_path_starts_with_slash():
    assert config.WS_PATH.startswith("/")
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `python -m pytest tests/test_lcd_viewer/test_config.py -v`
Expected: `5 passed` (config was written before test, so it passes — this is a "characterization" test that locks the constants).

- [ ] **Step 5: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/__init__.py tools/lcd_viewer/__init__.py tools/lcd_viewer/config.py tests/__init__.py tests/test_lcd_viewer/__init__.py tests/test_lcd_viewer/test_config.py
git commit -m "feat(lcd_viewer): scaffold package + config constants"
```

---

### Task 2: `frame.py` — pure parsing logic (TDD)

**Files:**
- Create: `tools/lcd_viewer/frame.py`
- Create: `tests/test_lcd_viewer/test_frame.py`

- [ ] **Step 1: Write the failing test `tests/test_lcd_viewer/test_frame.py`**

```python
"""Tests for tools.lcd_viewer.frame — JSON line → Frame."""
import base64
import json

import pytest

from tools.lcd_viewer import config
from tools.lcd_viewer.frame import Frame, parse_dump_memory_json


def _make_block(offset: int, size: int) -> dict:
    """Build a synthetic B1 block: `size` bytes of 0xAB, base64-encoded."""
    payload = bytes([0xAB] * size)
    return {
        "offset": offset,
        "size": size,
        "payload_b64": base64.b64encode(payload).decode("ascii"),
    }


def _make_b1_line(ts_us: int = 12345) -> str:
    """Build a synthetic dump-memory JSON line for a 4800-byte frame.

    Mirrors the B1 chunked format: 3 blocks of 2048+2048+704.
    """
    blocks = [_make_block(0, 2048), _make_block(2048, 2048), _make_block(4096, 704)]
    return json.dumps({"ts_us": ts_us, "blocks": blocks})


def test_parse_normal_b1_frame():
    line = _make_b1_line(ts_us=42)
    frame = parse_dump_memory_json(line)
    assert isinstance(frame, Frame)
    assert frame.ts_us == 42
    assert len(frame.payload) == config.FB_SIZE
    assert frame.payload[:8] == b"\xab" * 8


def test_parse_preserves_block_order_via_offset():
    """Even if blocks arrive in a different order, payload must be contiguous."""
    blocks = [_make_block(4096, 704), _make_block(0, 2048), _make_block(2048, 2048)]
    line = json.dumps({"ts_us": 7, "blocks": blocks})
    frame = parse_dump_memory_json(line)
    assert len(frame.payload) == config.FB_SIZE
    # 4800 bytes total; first 8 are 0xAB.
    assert frame.payload[0] == 0xAB
    # 2048th byte is start of 2nd block — still 0xAB.
    assert frame.payload[2048] == 0xAB
    # 4096th byte is start of 3rd block — still 0xAB.
    assert frame.payload[4096] == 0xAB


def test_parse_rejects_short_payload():
    blocks = [_make_block(0, 2048), _make_block(2048, 2048)]  # only 4096 bytes
    line = json.dumps({"ts_us": 1, "blocks": blocks})
    with pytest.raises(AssertionError, match="unexpected size"):
        parse_dump_memory_json(line)


def test_parse_rejects_oversized_payload():
    blocks = [_make_block(0, 2048), _make_block(2048, 2048), _make_block(4096, 705)]
    line = json.dumps({"ts_us": 1, "blocks": blocks})
    with pytest.raises(AssertionError, match="unexpected size"):
        parse_dump_memory_json(line)


def test_parse_rejects_malformed_json():
    with pytest.raises(json.JSONDecodeError):
        parse_dump_memory_json("not json at all")


def test_frame_is_immutable_dataclass():
    """Frame must be hashable / immutable so it can be cached safely."""
    f = Frame(ts_us=1, payload=b"\x00" * config.FB_SIZE)
    with pytest.raises(Exception):
        f.ts_us = 2  # frozen dataclass should reject this
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_lcd_viewer/test_frame.py -v`
Expected: `ModuleNotFoundError: No module named 'tools.lcd_viewer.frame'`

- [ ] **Step 3: Write minimal implementation `tools/lcd_viewer/frame.py`**

```python
"""Parse one JSON line from `python -m mklink dump-memory --json` into a Frame.

A `dump-memory` B1 frame looks like:
    {"ts_us": 12345, "blocks": [
        {"offset": 0,    "size": 2048, "payload_b64": "..."},
        {"offset": 2048, "size": 2048, "payload_b64": "..."},
        {"offset": 4096, "size":  704, "payload_b64": "..."}
    ]}

We sort blocks by `offset` (defensive — protocol says they arrive in order)
and concatenate the base64-decoded payloads.
"""
from __future__ import annotations

import base64
import json
from dataclasses import dataclass

from tools.lcd_viewer import config


@dataclass(frozen=True)
class Frame:
    """A single framebuffer snapshot.

    `ts_us` is the device's microsecond timestamp at the moment the snapshot
    was taken. `payload` is exactly `config.FB_SIZE` raw bytes.
    """
    ts_us: int
    payload: bytes


def parse_dump_memory_json(line: str) -> Frame:
    """Parse one stdout line from `dump-memory --json` into a Frame.

    Raises AssertionError if the assembled payload is not exactly FB_SIZE.
    Raises json.JSONDecodeError if the line is not valid JSON.
    """
    obj = json.loads(line)
    ts_us: int = obj["ts_us"]
    blocks = sorted(obj["blocks"], key=lambda b: b["offset"])
    buf = bytearray()
    for blk in blocks:
        buf.extend(base64.b64decode(blk["payload_b64"]))
    assert len(buf) == config.FB_SIZE, (
        f"unexpected size {len(buf)} (expected {config.FB_SIZE})"
    )
    return Frame(ts_us=ts_us, payload=bytes(buf))
```

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_lcd_viewer/test_frame.py -v`
Expected: `6 passed`

- [ ] **Step 5: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/frame.py tests/test_lcd_viewer/test_frame.py
git commit -m "feat(lcd_viewer): add Frame dataclass + dump-memory JSON parser"
```

---

### Task 3: `dumper.py` — subprocess management (TDD, with mock)

**Files:**
- Create: `tools/lcd_viewer/dumper.py`
- Create: `tests/test_lcd_viewer/test_dumper.py`

This is the riskiest module because it touches subprocess + threading. We mock `subprocess.Popen` and drive its `stdout` line by line.

- [ ] **Step 1: Write the failing test `tests/test_lcd_viewer/test_dumper.py`**

```python
"""Tests for tools.lcd_viewer.dumper — subprocess lifecycle + frame dispatch.

We patch `subprocess.Popen` to return a mock whose `.stdout` is an iterable
of strings (JSON lines), simulating dump-memory's output. We then drive
the Dumper through start → frames → stop and assert state changes.
"""
from __future__ import annotations

import asyncio
import base64
import json
import sys
from unittest.mock import MagicMock, patch

import pytest

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper, DumperState
from tools.lcd_viewer.frame import Frame


def _make_json_line(ts_us: int = 100) -> str:
    blocks = [
        {"offset": 0, "size": 2048, "payload_b64": base64.b64encode(b"\x00" * 2048).decode()},
        {"offset": 2048, "size": 2048, "payload_b64": base64.b64encode(b"\x00" * 2048).decode()},
        {"offset": 4096, "size": 704, "payload_b64": base64.b64encode(b"\x00" * 704).decode()},
    ]
    return json.dumps({"ts_us": ts_us, "blocks": blocks})


def _make_mock_proc(stdout_lines: list[str], returncode: int = 0) -> MagicMock:
    """Build a mock subprocess.Popen that yields the given stdout lines then exits."""
    proc = MagicMock()
    proc.stdout = iter(stdout_lines)  # blocking iterator
    proc.poll.return_value = None
    proc.returncode = returncode
    proc.wait.return_value = returncode
    return proc


@pytest.mark.asyncio
async def test_dumper_starts_in_idle_state():
    d = Dumper()
    assert d.state == DumperState.IDLE
    assert d.latest_frame is None


@pytest.mark.asyncio
async def test_dumper_transitions_to_running_on_first_frame():
    state_changes: list[tuple[DumperState, str]] = []

    def on_state(state, msg):
        state_changes.append((state, msg))

    d = Dumper(on_state_change=on_state)
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([_make_json_line(123)])
        await d.start(period_ms=50)
        # Allow the reader task to process one frame.
        await asyncio.sleep(0.05)

    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 123
    assert d.state == DumperState.RUNNING
    assert any(s == DumperState.STARTING for s, _ in state_changes)
    assert any(s == DumperState.RUNNING for s, _ in state_changes)

    await d.stop()


@pytest.mark.asyncio
async def test_dumper_stop_terminates_subprocess():
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_proc = _make_mock_proc([])  # empty stdout
        mock_popen.return_value = mock_proc
        await d.start(period_ms=50)
        await d.stop()
    # Graceful exit path: proc.wait() returns 0 → no kill needed.
    mock_proc.kill.assert_not_called()
    assert d.state == DumperState.IDLE


@pytest.mark.asyncio
async def test_dumper_force_kills_on_timeout():
    """If the subprocess doesn't exit within 2s, Popen.kill() must be called."""
    import subprocess

    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_proc = _make_mock_proc([])
        mock_proc.wait.side_effect = subprocess.TimeoutExpired(cmd="x", timeout=2.0)
        mock_popen.return_value = mock_proc
        await d.start(period_ms=50)
        await d.stop()
    mock_proc.kill.assert_called_once()
    # After kill, wait is called again to reap the zombie.
    assert mock_proc.wait.call_count >= 2


@pytest.mark.asyncio
async def test_dumper_handles_parse_errors_gracefully():
    """A malformed JSON line must not crash the reader; the next good line wins."""
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([
            "not json",                       # ignored
            _make_json_line(ts_us=99),        # accepted
        ])
        await d.start(period_ms=50)
        await asyncio.sleep(0.05)
    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 99
    await d.stop()


@pytest.mark.asyncio
async def test_dumper_restart_replaces_subprocess():
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([])
        await d.start(period_ms=50)
        first_proc = mock_popen.return_value
        # Calling start again with different period triggers stop+start.
        mock_popen.return_value = _make_mock_proc([])
        await d.start(period_ms=100)
        second_proc = mock_popen.return_value
    assert first_proc is not second_proc
    # Two different Popen() calls were made.
    assert mock_popen.call_count == 2
    await d.stop()


def test_dumper_state_enum_has_required_values():
    assert DumperState.IDLE
    assert DumperState.STARTING
    assert DumperState.RUNNING
    assert DumperState.STOPPING
    assert DumperState.ERROR
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_lcd_viewer/test_dumper.py -v`
Expected: `ModuleNotFoundError: No module named 'tools.lcd_viewer.dumper'`

- [ ] **Step 3: Write minimal implementation `tools/lcd_viewer/dumper.py`**

```python
"""Dumper: spawns `mklink dump-memory` and dispatches frames to a callback.

Responsibilities:
- Manage a long-lived subprocess (start, restart, stop, force-kill on timeout).
- Read its stdout line-by-line and parse each line into a Frame.
- Maintain `latest_frame` so the WS server can grab the newest snapshot.
- Emit state-change events so the UI can show "starting / running / error".

NOT responsible for: WebSocket, HTTP, frontend.
"""
from __future__ import annotations

import asyncio
import enum
import logging
import queue as queue_mod
import subprocess
import sys
import threading
from typing import Callable

from tools.lcd_viewer import config
from tools.lcd_viewer.frame import Frame, parse_dump_memory_json

log = logging.getLogger("lcd_viewer.dumper")


class DumperState(enum.Enum):
    IDLE = "idle"
    STARTING = "starting"
    RUNNING = "running"
    STOPPING = "stopping"
    ERROR = "error"


StateCallback = Callable[[DumperState, str], None]


class Dumper:
    """Owns one `mklink dump-memory` subprocess at a time."""

    def __init__(self, on_state_change: StateCallback | None = None) -> None:
        self._on_state = on_state_change or (lambda s, m: None)
        self._proc: subprocess.Popen | None = None
        self._reader_task: asyncio.Task | None = None
        self._reader_thread: threading.Thread | None = None
        self._latest: Frame | None = None
        self._state: DumperState = DumperState.IDLE
        self._lock = asyncio.Lock()
        self._loop: asyncio.AbstractEventLoop | None = None

    # ----- public properties -----

    @property
    def state(self) -> DumperState:
        return self._state

    @property
    def latest_frame(self) -> Frame | None:
        return self._latest

    def set_state_callback(self, cb: StateCallback) -> None:
        """Replace the state-change callback (used by WSServer)."""
        self._on_state = cb

    # ----- state transitions -----

    def _set_state(self, new: DumperState, msg: str = "") -> None:
        if new != self._state:
            self._state = new
            log.info("state → %s (%s)", new.value, msg)
            try:
                self._on_state(new, msg)
            except Exception:  # callback errors must not break us
                log.exception("on_state_change callback raised")

    # ----- public lifecycle -----

    async def start(self, period_ms: int) -> None:
        """Spawn the subprocess (or restart with new period if already running)."""
        async with self._lock:
            if self._proc and self._proc.poll() is None:
                # Already running — restart with new period.
                await self._stop_locked()
            self._loop = asyncio.get_running_loop()
            period_s = period_ms / 1000.0
            cmd = [
                sys.executable, "-m", "mklink", "dump-memory",
                f"{config.FB_ADDR:#x}:{config.FB_SIZE}",
                "--period", str(period_s),
                "--frames", "0",
                "--duration", "0",
                "--json",
            ]
            log.info("spawn: %s", " ".join(cmd))
            self._set_state(DumperState.STARTING, "spawning dump-memory")
            try:
                self._proc = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.DEVNULL,
                    text=True,
                    bufsize=1,  # line-buffered
                )
            except FileNotFoundError as e:
                self._set_state(DumperState.ERROR, f"mklink not found: {e}")
                return
            self._reader_task = asyncio.create_task(self._reader())

    async def stop(self) -> None:
        async with self._lock:
            await self._stop_locked()

    async def _stop_locked(self) -> None:
        if self._proc is None:
            self._set_state(DumperState.IDLE)
            return
        self._set_state(DumperState.STOPPING, "terminating subprocess")
        # Try to close stdout first to unblock the reader.
        if self._proc.stdout:
            try:
                self._proc.stdout.close()
            except Exception:
                pass
        # Wait up to 2s for graceful exit.
        try:
            await asyncio.get_running_loop().run_in_executor(
                None, self._proc.wait, 2.0
            )
        except subprocess.TimeoutExpired:
            log.warning("subprocess did not exit in 2s; killing")
            self._proc.kill()
            try:
                await asyncio.get_running_loop().run_in_executor(
                    None, self._proc.wait, 2.0
                )
            except Exception:
                pass
        if self._reader_task and not self._reader_task.done():
            self._reader_task.cancel()
            try:
                await self._reader_task
            except (asyncio.CancelledError, Exception):
                pass
        rc = self._proc.returncode
        self._proc = None
        self._reader_task = None
        self._latest = None
        if rc not in (0, None):
            self._set_state(DumperState.ERROR, f"subprocess exit code {rc}")
        else:
            self._set_state(DumperState.IDLE, "subprocess terminated")

    # ----- reader coroutine -----

    async def _reader(self) -> None:
        """Read stdout lines from a background thread, dispatch into event loop.

        We can't await subprocess.stdout directly (it's a blocking pipe),
        so we run a daemon thread that pushes each line into a thread-safe
        Queue. The async coroutine consumes from that queue via
        `loop.run_in_executor(None, q.get)` with a short timeout to remain
        responsive to cancellation.
        """
        assert self._proc is not None
        assert self._proc.stdout is not None
        line_queue: queue_mod.Queue[str | None] = queue_mod.Queue()

        def thread_main() -> None:
            try:
                for line in self._proc.stdout:  # type: ignore[union-attr]
                    line_queue.put(line)
            except Exception as e:
                log.warning("stdout reader thread error: %s", e)
            finally:
                line_queue.put(None)  # sentinel: EOF

        self._reader_thread = threading.Thread(target=thread_main, daemon=True)
        self._reader_thread.start()
        loop = asyncio.get_running_loop()

        try:
            while True:
                # Pull one line in a worker thread (avoids blocking the loop).
                line = await loop.run_in_executor(None, line_queue.get)
                if line is None:  # EOF
                    break
                if not line.strip():
                    continue
                try:
                    frame = parse_dump_memory_json(line)
                except Exception as e:
                    log.warning("parse error: %s", e)
                    continue
                self._latest = frame
                if self._state == DumperState.STARTING:
                    self._set_state(DumperState.RUNNING, "first frame received")
        except Exception as e:
            self._set_state(DumperState.ERROR, f"reader crashed: {e}")
        finally:
            if self._proc and self._proc.poll() is not None and self._proc.returncode != 0:
                self._set_state(
                    DumperState.ERROR, f"subprocess exit code {self._proc.returncode}"
                )
```

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_lcd_viewer/test_dumper.py -v`
Expected: `7 passed` (the `test_dumper_force_kills_on_timeout` may need a small sleep — the test patches `wait.side_effect` which works synchronously).

If the asyncio marker isn't recognized, add `asyncio_mode = "auto"` to `pyproject.toml` (or a `pytest.ini`):

```ini
# pytest.ini  (create at repo root if missing)
[pytest]
asyncio_mode = auto
```

- [ ] **Step 5: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/dumper.py tests/test_lcd_viewer/test_dumper.py
git commit -m "feat(lcd_viewer): add Dumper class (subprocess mgmt + frame dispatch + state machine)"
```

---

## Phase 2: WebSocket Server

### Task 4: `ws_server.py` — WebSocket + static files (TDD)

**Files:**
- Create: `tools/lcd_viewer/ws_server.py`
- Create: `tests/test_lcd_viewer/test_ws_server.py`

- [ ] **Step 1: Write the failing test `tests/test_lcd_viewer/test_ws_server.py`**

```python
"""Tests for tools.lcd_viewer.ws_server.

We exercise the WebSocket endpoint by starting the server in a background
task and connecting with the `websockets` client. The Dumper is replaced
by a fake that yields a single canned frame on demand.
"""
from __future__ import annotations

import asyncio
import base64
import json
from typing import Iterator

import pytest
import websockets

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import DumperState
from tools.lcd_viewer.frame import Frame
from tools.lcd_viewer.ws_server import WSServer


class FakeDumper:
    """A drop-in replacement for Dumper for tests.

    It exposes the same `state` property, `latest_frame`, `start()`, `stop()`.
    It also lets tests push a frame manually via `push_frame()`.
    """
    def __init__(self) -> None:
        self.state: DumperState = DumperState.IDLE
        self.latest_frame: Frame | None = None
        self._state_cb = lambda s, m: None
        self._subscribers: list[asyncio.Queue] = []

    def set_state_callback(self, cb):
        self._state_cb = cb

    async def start(self, period_ms: int):
        self.state = DumperState.STARTING
        self._state_cb(self.state, "starting")
        self.state = DumperState.RUNNING
        self._state_cb(self.state, "running")

    async def stop(self):
        self.state = DumperState.STOPPING
        self._state_cb(self.state, "stopping")
        self.state = DumperState.IDLE
        self._state_cb(self.state, "idle")

    def push_frame(self, frame: Frame) -> None:
        self.latest_frame = frame


def _make_canned_frame(ts_us: int = 999) -> Frame:
    payload = b"\xab" * config.FB_SIZE
    return Frame(ts_us=ts_us, payload=payload)


@pytest.mark.asyncio
async def test_hello_message_sent_on_connect():
    server = WSServer(dumper=FakeDumper())  # type: ignore[arg-type]
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=18765))
    await asyncio.sleep(0.1)  # let the server bind
    try:
        async with websockets.connect("ws://127.0.0.1:18765/ws") as ws:
            hello = json.loads(await asyncio.wait_for(ws.recv(), timeout=2.0))
            assert hello["type"] == "hello"
            assert hello["fb_w"] == config.FB_WIDTH
            assert hello["fb_h"] == config.FB_HEIGHT
    finally:
        await server.stop()
        server_task.cancel()
        try:
            await server_task
        except (asyncio.CancelledError, Exception):
            pass


@pytest.mark.asyncio
async def test_client_count_increments_and_decrements():
    dumper = FakeDumper()
    server = WSServer(dumper=dumper)  # type: ignore[arg-type]
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=18766))
    await asyncio.sleep(0.1)
    try:
        assert server.client_count == 0
        async with websockets.connect("ws://127.0.0.1:18766/ws"):
            await asyncio.sleep(0.05)
            assert server.client_count == 1
            # First client → dumper.start should have been called.
            assert dumper.state == DumperState.RUNNING
        await asyncio.sleep(0.1)
        # Last client gone → dumper.stop should have been called.
        assert server.client_count == 0
        assert dumper.state == DumperState.IDLE
    finally:
        await server.stop()
        server_task.cancel()
        try:
            await server_task
        except (asyncio.CancelledError, Exception):
            pass


@pytest.mark.asyncio
async def test_dumper_not_restarted_when_second_client_connects():
    """When the dumper is already running, a 2nd client must NOT trigger start() again."""
    dumper = FakeDumper()
    server = WSServer(dumper=dumper)  # type: ignore[arg-type]
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=18767))
    await asyncio.sleep(0.1)
    start_call_count = 0
    original_start = dumper.start

    async def counting_start(period_ms: int):
        nonlocal start_call_count
        start_call_count += 1
        await original_start(period_ms)

    dumper.start = counting_start  # type: ignore[method-assign]
    try:
        async with websockets.connect("ws://127.0.0.1:18767/ws") as ws1:
            await asyncio.sleep(0.05)
            async with websockets.connect("ws://127.0.0.1:18767/ws") as ws2:
                await asyncio.sleep(0.05)
                assert server.client_count == 2
                # Only one start() call should have happened (from client 1).
                assert start_call_count == 1
    finally:
        await server.stop()
        server_task.cancel()
        try:
            await server_task
        except (asyncio.CancelledError, Exception):
            pass
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python -m pytest tests/test_lcd_viewer/test_ws_server.py -v`
Expected: `ModuleNotFoundError: No module named 'tools.lcd_viewer.ws_server'`

- [ ] **Step 3: Write minimal implementation `tools/lcd_viewer/ws_server.py`**

```python
"""WebSocket + static-file server for the LCD framebuffer web viewer.

Layout:
- HTTP GET /            → serves web/index.html
- HTTP GET /<file>      → serves web/<file> (app.js, etc.)
- WS    /ws             → streams 4800-byte binary frames + JSON control

The server owns a Dumper. It starts the dumper when the first client
connects, and stops it when the last client disconnects.
"""
from __future__ import annotations

import asyncio
import json
import logging
import os
import pathlib
from typing import Any, Protocol

import websockets
from websockets.asyncio.server import ServerConnection, serve

from tools.lcd_viewer import config

log = logging.getLogger("lcd_viewer.ws_server")


class DumperLike(Protocol):
    """Anything that quacks like a Dumper — kept loose for testing."""
    state: Any
    latest_frame: Any

    async def start(self, period_ms: int) -> None: ...
    async def stop(self) -> None: ...
    def set_state_callback(self, cb) -> None: ...


class WSServer:
    """WebSocket server + static HTTP file server.

    One instance = one TCP listener. Multiple clients share the same
    Dumper (started on first connect, stopped on last disconnect).
    """

    def __init__(self, dumper: DumperLike) -> None:
        self._dumper = dumper
        self._ws_server: Any = None
        self._clients: set[ServerConnection] = set()
        self._client_count = 0
        self._lock = asyncio.Lock()
        self._state_task: asyncio.Task | None = None
        # Static file directory.
        self._web_dir = pathlib.Path(__file__).parent / "web"
        # dumper → ws_server state broadcast.
        self._dumper.set_state_callback(self._on_dumper_state)
        # Default period for first start.
        self._period_ms: int = config.DEFAULT_PERIOD_MS

    @property
    def client_count(self) -> int:
        return self._client_count

    # ----- lifecycle -----

    async def start(self, host: str = config.HOST, port: int = config.PORT) -> None:
        self._ws_server = await serve(
            self._handler, host, port, ping_interval=20, ping_timeout=20
        )
        log.info("listening on ws://%s:%d%s", host, port, config.WS_PATH)

    async def stop(self) -> None:
        if self._ws_server is not None:
            self._ws_server.close()
            await self._ws_server.wait_closed()
            self._ws_server = None

    async def serve_forever(self) -> None:
        if self._ws_server is not None:
            await self._ws_server.wait_closed()

    # ----- connection handler -----

    async def _handler(self, connection: ServerConnection) -> None:
        path = connection.request.path if hasattr(connection, "request") else "/"
        log.info("client connected: %s %s", connection.remote_address, path)

        # Route: /ws → WebSocket. Anything else → static file.
        if path == config.WS_PATH:
            await self._ws_loop(connection)
        else:
            await self._serve_http(connection, path)

    # ----- HTTP static files -----

    async def _serve_http(self, connection: ServerConnection, path: str) -> None:
        # Map "/" → index.html; otherwise strip leading "/".
        if path in ("/", ""):
            rel = "index.html"
        else:
            rel = path.lstrip("/")
        # Prevent path traversal.
        target = (self._web_dir / rel).resolve()
        if not str(target).startswith(str(self._web_dir.resolve())):
            await self._http_response(connection, 403, b"forbidden", "text/plain")
            return
        if not target.is_file():
            await self._http_response(connection, 404, b"not found", "text/plain")
            return
        ext = target.suffix.lower()
        ctype = {
            ".html": "text/html; charset=utf-8",
            ".js": "application/javascript; charset=utf-8",
            ".css": "text/css; charset=utf-8",
            ".png": "image/png",
            ".svg": "image/svg+xml",
        }.get(ext, "application/octet-stream")
        body = target.read_bytes()
        await self._http_response(connection, 200, body, ctype)

    @staticmethod
    async def _http_response(connection: ServerConnection, status: int, body: bytes, ctype: str) -> None:
        reason = {200: "OK", 403: "Forbidden", 404: "Not Found"}.get(status, "OK")
        headers = [
            ("Content-Type", ctype),
            ("Content-Length", str(len(body))),
            ("Connection", "close"),
        ]
        # websockets lib exposes `connection.transport` for raw HTTP responses.
        transport = connection.transport
        if transport is None:
            return
        transport.write(
            f"HTTP/1.1 {status} {reason}\r\n".encode("ascii")
            + b"".join(f"{k}: {v}\r\n".encode("ascii") for k, v in headers)
            + b"\r\n"
            + body
        )
        try:
            await connection.close()
        except Exception:
            pass

    # ----- WebSocket protocol -----

    async def _ws_loop(self, connection: ServerConnection) -> None:
        # First client triggers dumper start.
        await self._register_client(connection)
        try:
            # Send hello.
            await connection.send(json.dumps({
                "type": "hello",
                "fb_w": config.FB_WIDTH,
                "fb_h": config.FB_HEIGHT,
                "fb_size": config.FB_SIZE,
                "period_ms": self._period_ms,
            }).encode("utf-8"))
            # Receive control messages until disconnect.
            async for raw in connection:
                if isinstance(raw, str):
                    await self._handle_control(connection, raw)
                # Binary frames from client are not used; ignore.
        except websockets.ConnectionClosed:
            pass
        finally:
            await self._unregister_client(connection)

    async def _handle_control(self, connection: ServerConnection, raw: str) -> None:
        try:
            msg = json.loads(raw)
        except json.JSONDecodeError:
            return
        kind = msg.get("type")
        if kind == "set_period":
            new_ms = int(msg.get("ms", config.DEFAULT_PERIOD_MS))
            new_ms = max(config.MIN_PERIOD_MS, min(config.MAX_PERIOD_MS, new_ms))
            self._period_ms = new_ms
            log.info("period → %d ms", new_ms)
            # Restart dumper with new period.
            await self._dumper.start(new_ms)
        elif kind == "pause":
            # Pause is client-side rendering; we just ACK by sending status.
            await self._broadcast_status("running", "client paused")
        elif kind == "resume":
            await self._broadcast_status("running", "client resumed")
        elif kind == "save_png":
            await self._send_png(connection)

    async def _send_png(self, connection: ServerConnection) -> None:
        frame = self._dumper.latest_frame
        if frame is None:
            return
        # Convert 4800 bytes → 240x160 mode-'1' PIL Image → PNG.
        try:
            from PIL import Image
            import io
            img = Image.frombytes("1", (config.FB_WIDTH, config.FB_HEIGHT), frame.payload)
            buf = io.BytesIO()
            img.save(buf, format="PNG")
            png_bytes = buf.getvalue()
            # Frame: [4B LE uint32 length][PNG bytes]
            import struct
            await connection.send(struct.pack("<I", len(png_bytes)) + png_bytes)
        except Exception as e:
            log.warning("PNG save failed: %s", e)

    # ----- client registration / broadcast -----

    async def _register_client(self, connection: ServerConnection) -> None:
        async with self._lock:
            self._clients.add(connection)
            was_zero = self._client_count == 0
            self._client_count += 1
            if was_zero:
                log.info("first client — starting dumper")
                await self._dumper.start(self._period_ms)
                # Spawn the broadcast loop if not running.
                if self._state_task is None or self._state_task.done():
                    self._state_task = asyncio.create_task(self._broadcast_loop())

    async def _unregister_client(self, connection: ServerConnection) -> None:
        async with self._lock:
            self._clients.discard(connection)
            self._client_count -= 1
            if self._client_count <= 0:
                self._client_count = 0
                log.info("last client gone — stopping dumper")
                await self._dumper.stop()

    async def _broadcast_loop(self) -> None:
        """Continuously push the latest frame to all clients.

        Polls the dumper's `latest_frame` reference. Whenever it changes,
        send the new frame to every connected client as a binary message.
        """
        last_sent_ts: int = -1
        while self._client_count > 0:
            frame = self._dumper.latest_frame
            if frame is not None and frame.ts_us != last_sent_ts:
                last_sent_ts = frame.ts_us
                # Snapshot the client set; copy to avoid mutation during send.
                targets = list(self._clients)
                for c in targets:
                    try:
                        await c.send(frame.payload)
                    except Exception:
                        pass  # client will be cleaned up on next event
            await asyncio.sleep(0.005)  # 200 Hz poll — cheap, avoids busy-wait

    async def _broadcast_status(self, state: str, msg: str) -> None:
        text = json.dumps({"type": "status", "state": state, "msg": msg}).encode("utf-8")
        for c in list(self._clients):
            try:
                await c.send(text)
            except Exception:
                pass

    def _on_dumper_state(self, state, msg: str) -> None:
        """Called by the dumper on state changes. Schedule a status broadcast."""
        try:
            loop = asyncio.get_running_loop()
            loop.create_task(self._broadcast_status(state.value, msg))
        except RuntimeError:
            pass  # no running loop yet
```

- [ ] **Step 4: Run test to verify it passes**

Run: `python -m pytest tests/test_lcd_viewer/test_ws_server.py -v`
Expected: `3 passed`

If the `path` attribute on `ServerConnection` is named differently in your websockets version, look it up via `vars(connection)` and adjust. The tests will tell you.

- [ ] **Step 5: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/ws_server.py tests/test_lcd_viewer/test_ws_server.py
git commit -m "feat(lcd_viewer): add WSServer (static files + WebSocket protocol + client count)"
```

---

### Task 5: `server.py` — CLI entry point

**Files:**
- Create: `tools/lcd_viewer/server.py`

- [ ] **Step 1: Write `tools/lcd_viewer/server.py`**

```python
"""CLI entry point for the LCD framebuffer web viewer.

Usage:
    python tools/lcd_viewer/server.py [--port 8765] [--period 50] [--host 127.0.0.1]

Then open http://127.0.0.1:8765/ in a browser.
"""
from __future__ import annotations

import argparse
import asyncio
import logging
import signal
import sys

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper
from tools.lcd_viewer.ws_server import WSServer


def _parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(
        prog="lcd_viewer",
        description="Stream the LCD framebuffer to a local browser.",
    )
    p.add_argument("--host", default=config.HOST, help="bind host (default 127.0.0.1)")
    p.add_argument("--port", type=int, default=config.PORT, help=f"port (default {config.PORT})")
    p.add_argument(
        "--period",
        type=int,
        default=config.DEFAULT_PERIOD_MS,
        help=f"sampling period in ms (default {config.DEFAULT_PERIOD_MS})",
    )
    p.add_argument("-v", "--verbose", action="store_true", help="enable debug logging")
    return p.parse_args()


async def _main(args: argparse.Namespace) -> int:
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(name)s %(levelname)s %(message)s",
        stream=sys.stderr,
    )
    dumper = Dumper()
    server = WSServer(dumper)
    await server.start(host=args.host, port=args.port)
    print(f"[lcd_viewer] Open http://{args.host}:{args.port}/ in a browser", file=sys.stderr)
    print(f"[lcd_viewer] Default period: {args.period} ms", file=sys.stderr)
    # Wait until SIGINT / SIGTERM.
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, stop.set)
        except (NotImplementedError, RuntimeError):
            pass  # Windows: signal handlers may not be supported in all envs
    try:
        await stop.wait()
    finally:
        print("[lcd_viewer] shutting down...", file=sys.stderr)
        await server.stop()
        await dumper.stop()
    return 0


def main() -> int:
    args = _parse_args()
    return asyncio.run(_main(args))


if __name__ == "__main__":
    raise SystemExit(main())
```

- [ ] **Step 2: Smoke test — start the server, curl /, kill**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
python tools/lcd_viewer/server.py --port 18799 &
SERVER_PID=$!
sleep 1
# Should print "Open http://127.0.0.1:18799/" and bind a port.
# The index.html doesn't exist yet, so /  will return 404 — that's fine for this smoke test.
# What's important is the server starts and the WS endpoint can be hit.
# We can verify by killing cleanly:
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
```

Expected: server starts, prints the open URL, terminates cleanly on SIGTERM.

- [ ] **Step 3: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/server.py
git commit -m "feat(lcd_viewer): add CLI entry point server.py"
```

---

## Phase 3: Frontend (Vanilla HTML + JS)

### Task 6: `web/index.html` — page skeleton

**Files:**
- Create: `tools/lcd_viewer/web/index.html`

- [ ] **Step 1: Write `tools/lcd_viewer/web/index.html`**

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>MKLink LCD Viewer</title>
<style>
  * { margin: 0; padding: 0; box-sizing: border-box; }
  html, body {
    background: #0a0a14;
    color: #50ff78;
    font-family: 'Consolas', 'Menlo', 'Courier New', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 20px;
  }
  .header {
    width: 100%;
    max-width: 720px;
    text-align: center;
    margin-bottom: 16px;
    padding: 8px 12px;
    background: #111;
    border: 1px solid #1a3a1a;
    border-radius: 4px;
  }
  .header h1 {
    font-size: 14px;
    font-weight: normal;
    letter-spacing: 2px;
    color: #50ff78;
  }
  .stats {
    display: flex;
    gap: 18px;
    justify-content: center;
    margin-top: 6px;
    font-size: 11px;
    color: #888;
  }
  .stats b { color: #50ff78; font-weight: normal; }
  .canvas-wrap {
    background: #000;
    border: 1px solid #1a3a1a;
    box-shadow: 0 0 24px rgba(80, 255, 120, 0.10);
    padding: 8px;
  }
  canvas#lcd {
    display: block;
    image-rendering: pixelated;
    image-rendering: crisp-edges;
    background: #000;
  }
  .controls {
    width: 100%;
    max-width: 720px;
    margin-top: 16px;
    padding: 10px 12px;
    background: #111;
    border: 1px solid #1a3a1a;
    border-radius: 4px;
    display: flex;
    gap: 10px;
    align-items: center;
    justify-content: center;
    font-size: 12px;
  }
  button, input[type="number"] {
    background: #000;
    color: #50ff78;
    border: 1px solid #1a3a1a;
    padding: 4px 10px;
    font-family: inherit;
    font-size: 12px;
    cursor: pointer;
  }
  button:hover { background: #1a3a1a; }
  input[type="number"] { width: 60px; text-align: center; }
  .status-dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #555;
    margin-right: 4px;
    vertical-align: middle;
  }
  .status-dot.running { background: #50ff78; }
  .status-dot.error   { background: #ff5050; }
  .status-dot.starting{ background: #ffd060; }
</style>
</head>
<body>
<div class="header">
  <h1>
    <span class="status-dot" id="status-dot"></span>
    GD32 LCD · <b id="status-text">connecting…</b>
  </h1>
  <div class="stats">
    分辨率 <b>240×160</b>
    · 实际 FPS <b id="fps">--</b>
    · 累计帧 <b id="frames">0</b>
    · 末帧 ts <b id="last-ts">--</b>
  </div>
</div>
<div class="canvas-wrap">
  <canvas id="lcd" width="240" height="160"></canvas>
</div>
<div class="controls">
  <button id="btn-pause">⏸ 暂停</button>
  <span>周期</span>
  <input type="number" id="period" min="10" max="200" step="5" value="50">
  <span>ms</span>
  <button id="btn-save">💾 保存当前帧</button>
</div>
<script src="app.js"></script>
</body>
</html>
```

- [ ] **Step 2: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/web/index.html
git commit -m "feat(lcd_viewer): add web/index.html skeleton (1:1 canvas + status bar + controls)"
```

---

### Task 7: `web/app.js` — WebSocket client + canvas + controls

**Files:**
- Create: `tools/lcd_viewer/web/app.js`

- [ ] **Step 1: Write `tools/lcd_viewer/web/app.js`**

```javascript
// MKLink LCD Viewer — frontend logic.
// Connects to ws://<host>/ws, draws 4800-byte binary frames onto a 240x160
// canvas at native 1:1, and wires up the period / pause / save controls.

(() => {
  "use strict";

  // ---------- DOM ----------
  const canvas    = document.getElementById("lcd");
  const ctx       = canvas.getContext("2d");
  const statusDot = document.getElementById("status-dot");
  const statusTxt = document.getElementById("status-text");
  const fpsEl     = document.getElementById("fps");
  const framesEl  = document.getElementById("frames");
  const tsEl      = document.getElementById("last-ts");
  const btnPause  = document.getElementById("btn-pause");
  const inPeriod  = document.getElementById("period");
  const btnSave   = document.getElementById("btn-save");

  const FB_W = 240, FB_H = 160, FB_SIZE = FB_W * FB_H / 8;  // 4800

  // Pre-allocate the ImageData buffer (240*160*4 bytes).
  const img = ctx.createImageData(FB_W, FB_H);

  // ---------- State ----------
  let paused = false;
  let totalFrames = 0;
  let fpsFrames = 0;
  let lastFpsTs = performance.now();
  let lastFrameTs = 0;        // device ts_us of most recent rendered frame
  let helloPeriodMs = 50;
  let pendingSaveBlobUrl = null;

  // ---------- WebSocket ----------
  function setStatus(state, msg) {
    statusDot.className = "status-dot " + (state || "");
    statusTxt.textContent = msg || state || "unknown";
  }

  function connect() {
    setStatus("", "connecting…");
    const proto = location.protocol === "https:" ? "wss" : "ws";
    const url = `${proto}://${location.host}/ws`;
    const ws = new WebSocket(url);
    ws.binaryType = "arraybuffer";

    ws.onopen = () => {
      setStatus("starting", "等待首帧…");
    };

    ws.onmessage = (ev) => {
      if (typeof ev.data === "string") {
        handleText(JSON.parse(ev.data));
      } else {
        handleBinary(new Uint8Array(ev.data));
      }
    };

    ws.onerror = () => {
      setStatus("error", "ws error");
    };

    ws.onclose = () => {
      setStatus("", "disconnected, retrying in 2s…");
      setTimeout(connect, 2000);
    };

    window._ws = ws;  // for debugging from devtools
  }

  function handleText(msg) {
    if (msg.type === "hello") {
      helloPeriodMs = msg.period_ms;
      inPeriod.value = helloPeriodMs;
    } else if (msg.type === "status") {
      setStatus(msg.state, msg.msg || msg.state);
    }
  }

  function handleBinary(bytes) {
    if (paused) return;
    if (bytes.length !== FB_SIZE) {
      console.warn("frame size mismatch:", bytes.length, "expected", FB_SIZE);
      return;
    }
    renderFrame(bytes);
    // Device ts is not currently pushed by the server (it only sends raw
    // bytes); we synthesize one from the local clock for display purposes.
    lastFrameTs = performance.now() * 1000;  // µs, monotonic
    totalFrames += 1;
    fpsFrames += 1;
    const now = performance.now();
    if (now - lastFpsTs >= 1000) {
      fpsEl.textContent = (fpsFrames * 1000 / (now - lastFpsTs)).toFixed(1);
      framesEl.textContent = totalFrames.toString();
      tsEl.textContent = Math.floor(lastFrameTs).toString();
      fpsFrames = 0;
      lastFpsTs = now;
    }
  }

  // ---------- Canvas rendering ----------
  // 1bpp MSB-first → 32-bit RGBA. bit=1 → bright green, bit=0 → black.
  function renderFrame(buf) {
    const px = img.data;
    let p = 0;
    for (let y = 0; y < FB_H; y++) {
      for (let xByte = 0; xByte < FB_W / 8; xByte++) {
        const b = buf[y * (FB_W / 8) + xByte];
        for (let bit = 7; bit >= 0; bit--) {
          const on = (b >> bit) & 1;
          px[p]     = on ? 0x50 : 0x00;
          px[p + 1] = on ? 0xff : 0x00;
          px[p + 2] = on ? 0x78 : 0x00;
          px[p + 3] = 0xff;
          p += 4;
        }
      }
    }
    ctx.putImageData(img, 0, 0);
  }

  // ---------- Controls ----------
  btnPause.addEventListener("click", () => {
    paused = !paused;
    btnPause.textContent = paused ? "▶ 继续" : "⏸ 暂停";
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: paused ? "pause" : "resume" }));
    }
  });

  inPeriod.addEventListener("change", () => {
    let v = parseInt(inPeriod.value, 10);
    if (isNaN(v)) v = 50;
    v = Math.max(10, Math.min(200, v));
    inPeriod.value = v;
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: "set_period", ms: v }));
    }
  });

  btnSave.addEventListener("click", () => {
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: "save_png" }));
      btnSave.textContent = "⏳ 生成中…";
      btnSave.disabled = true;
    }
  });

  // Listen for the PNG binary response (4B LE uint32 length + PNG bytes).
  // The browser fires `onmessage` again with binary type, so we need to
  // distinguish: we use a flag set by btnSave and reset it once we receive
  // a binary frame.
  let waitingForPng = false;
  btnSave.addEventListener("click", () => { waitingForPng = true; });
  // We need to override handleBinary temporarily. Easiest: hook the ws.
  const origConnect = connect;
  connect = function patchedConnect() {
    origConnect();
    // The connect function reassigns window._ws, so we add a one-shot
    // listener on the new socket.
    const oldOnmessage = window._ws.onmessage;
    window._ws.onmessage = (ev) => {
      oldOnmessage(ev);
      if (waitingForPng && ev.data instanceof ArrayBuffer) {
        waitingForPng = false;
        const view = new DataView(ev.data);
        const len = view.getUint32(0, true);
        const png = new Uint8Array(ev.data, 4, len);
        const blob = new Blob([png], { type: "image/png" });
        if (pendingSaveBlobUrl) URL.revokeObjectURL(pendingSaveBlobUrl);
        pendingSaveBlobUrl = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = pendingSaveBlobUrl;
        a.download = `frame_${Date.now()}.png`;
        document.body.appendChild(a);
        a.click();
        a.remove();
        btnSave.textContent = "💾 保存当前帧";
        btnSave.disabled = false;
      }
    };
  };

  // ---------- Go ----------
  connect();
})();
```

- [ ] **Step 2: Manual smoke test**

1. Start the server:
   ```bash
   cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
   python tools/lcd_viewer/server.py --port 8765
   ```
2. Open `http://127.0.0.1:8765/` in a browser.
3. You should see: a black 240×160 canvas, status dot, header.
4. Without a real device connected, you won't get frames (dumper will start and error out quickly, status: error). That's expected — full validation requires MCU.

- [ ] **Step 3: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/web/app.js
git commit -m "feat(lcd_viewer): add web/app.js (WS client, canvas renderer, controls, PNG save)"
```

---

## Phase 4: Integration + Documentation

### Task 8: Integration test (mock `mklink` end-to-end)

**Files:**
- Create: `tests/test_lcd_viewer/test_integration.py`

- [ ] **Step 1: Write the test `tests/test_lcd_viewer/test_integration.py`**

```python
"""End-to-end test: mock the dump-memory subprocess, drive the full pipeline.

Verifies that a frame emitted by the (mocked) subprocess arrives at a
WebSocket client as exactly 4800 bytes in the correct order.
"""
from __future__ import annotations

import asyncio
import base64
import json
import os
import sys
from unittest.mock import patch

import pytest
import websockets

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper
from tools.lcd_viewer.frame import Frame
from tools.lcd_viewer.ws_server import WSServer


def _json_line(ts_us: int, fill: int = 0x55) -> str:
    chunks = [(0, 2048), (2048, 2048), (4096, 704)]
    blocks = [
        {"offset": off, "size": sz, "payload_b64": base64.b64encode(bytes([fill] * sz)).decode()}
        for off, sz in chunks
    ]
    return json.dumps({"ts_us": ts_us, "blocks": blocks})


@pytest.mark.asyncio
async def test_end_to_end_frame_delivery():
    """Spawn a real Dumper (subprocess mocked) and verify WS clients receive frames."""
    dumper = Dumper()
    server = WSServer(dumper)
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=18999))
    await asyncio.sleep(0.1)

    # Drive 3 frames into the mocked dump-memory subprocess.
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_proc = mock_popen.return_value
        mock_proc.stdout = iter([_json_line(1, 0x11),
                                 _json_line(2, 0x22),
                                 _json_line(3, 0x33)])
        mock_proc.poll.return_value = None
        mock_proc.wait.return_value = 0

        try:
            async with websockets.connect("ws://127.0.0.1:18999/ws") as ws:
                hello = json.loads(await asyncio.wait_for(ws.recv(), 2.0))
                assert hello["type"] == "hello"

                # Collect 3 binary frames.
                received = []
                for _ in range(3):
                    raw = await asyncio.wait_for(ws.recv(), 2.0)
                    assert isinstance(raw, bytes)
                    assert len(raw) == config.FB_SIZE
                    received.append(raw)

                # Verify each frame's first byte matches its fill.
                assert received[0][0] == 0x11
                assert received[1][0] == 0x22
                assert received[2][0] == 0x33
        finally:
            await server.stop()
            await dumper.stop()
            server_task.cancel()
            try:
                await server_task
            except (asyncio.CancelledError, Exception):
                pass


@pytest.mark.asyncio
async def test_set_period_restarts_dumper():
    """A `set_period` control message must trigger dumper.start() with the new value."""
    dumper = Dumper()
    server = WSServer(dumper)
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=18998))
    await asyncio.sleep(0.1)
    start_calls: list[int] = []
    original_start = dumper.start

    async def tracking_start(period_ms: int):
        start_calls.append(period_ms)
        await original_start(period_ms)

    dumper.start = tracking_start  # type: ignore[method-assign]

    try:
        async with websockets.connect("ws://127.0.0.1:18998/ws") as ws:
            await asyncio.wait_for(ws.recv(), 2.0)  # hello
            assert start_calls == [50]  # initial start
            # Send set_period control.
            await ws.send(json.dumps({"type": "set_period", "ms": 100}))
            await asyncio.sleep(0.05)
            assert 100 in start_calls
    finally:
        await server.stop()
        await dumper.stop()
        server_task.cancel()
        try:
            await server_task
        except (asyncio.CancelledError, Exception):
            pass
```

- [ ] **Step 2: Run the tests**

Run: `python -m pytest tests/test_lcd_viewer/test_integration.py -v`
Expected: `2 passed`

- [ ] **Step 3: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tests/test_lcd_viewer/test_integration.py
git commit -m "test(lcd_viewer): add end-to-end integration test (mocked subprocess)"
```

---

### Task 9: `README.md` + run full test suite

**Files:**
- Create: `tools/lcd_viewer/README.md`

- [ ] **Step 1: Write `tools/lcd_viewer/README.md`**

````markdown
# LCD Framebuffer Web Viewer

A localhost web viewer that streams the MCU's `framebuffer` (240×160 @ 1bpp, 4800 bytes) to a browser at ~20 fps via WebSocket.

## What it does

- Spawns `python -m mklink dump-memory 0x20001ba4:4800 --period 50ms --json` as a subprocess.
- Parses the B1 chunked JSON frames.
- Broadcasts the 4800-byte payload to all connected WebSocket clients.
- Renders each frame on a 240×160 `<canvas>` (1:1 native size, pixelated).

## Requirements

- Python 3.10+
- `pip install -e .` from the project root (installs `mklink` CLI as a module).
- `websockets` library (already in the project).
- `Pillow` (already in the project).
- An MKLink probe connected to the MCU's SWD pins.

## Quick start

```bash
# from project root
python tools/lcd_viewer/server.py
# Open http://127.0.0.1:8765/ in a browser.
```

You should see a black 240×160 canvas. The first WebSocket client connection triggers the dump-memory subprocess; the last disconnect stops it.

## CLI options

| Flag | Default | Description |
|------|---------|-------------|
| `--host` | `127.0.0.1` | Bind host. Set to `0.0.0.0` for LAN access. |
| `--port` | `8765` | HTTP/WS port. |
| `--period` | `50` | Sampling period in ms (10–200). Can also be changed live in the browser. |
| `-v` | off | Debug logging to stderr. |

## Web UI

- **Status bar** (top): connection dot, FPS, total frames, last frame timestamp.
- **Canvas** (center): 240×160 LCD, native 1:1.
- **Controls** (bottom):
  - `⏸ 暂停` / `▶ 继续` — pause/resume client-side rendering.
  - `周期 [N] ms` — change sampling period (server restarts the dumper).
  - `💾 保存当前帧` — download the most recent frame as PNG.

## Tests

```bash
python -m pytest tests/test_lcd_viewer/ -v
```

Integration tests use `subprocess.Popen` mocks — no hardware required.

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Status dot stays gray | WebSocket can't connect | Check `mklink.config.json` has the right COM port. |
| Status dot turns red `error` | dump-memory failed to start | Run `python -m mklink project-info` to verify. |
| Black canvas with red dot | MCU not responding | Check SWD cable, power, MCU reset. |
| FPS shows ~5 instead of ~20 | USB CDC bottleneck | Try `--period 100` to halve the load. |
| Browser shows "disconnected" | Server stopped | Restart with `python tools/lcd_viewer/server.py`. |

## File map

| File | Role |
|------|------|
| `config.py` | Constants: address, size, period range, port |
| `frame.py` | `Frame` dataclass + JSON line parser |
| `dumper.py` | Subprocess lifecycle + state machine |
| `ws_server.py` | WebSocket protocol + static file serving |
| `server.py` | CLI entry point |
| `web/index.html` | Page layout + CSS |
| `web/app.js` | WebSocket client + canvas renderer + controls |

## Limitations (out of scope)

- Read-only — no key injection or display control.
- Single framebuffer region — only `framebuffer` is read.
- Localhost only by default.
- No recording / playback.
````

- [ ] **Step 2: Run the full test suite to verify everything still passes**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
python -m pytest tests/test_lcd_viewer/ -v
```

Expected: all tests pass. Count should be:
- test_config.py: 5
- test_frame.py: 6
- test_dumper.py: 7
- test_ws_server.py: 3
- test_integration.py: 2
- **Total: 23 passed**

- [ ] **Step 3: Commit**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add tools/lcd_viewer/README.md
git commit -m "docs(lcd_viewer): add README with usage, CLI, troubleshooting"
```

---

### Task 10: Manual end-to-end smoke test (with real hardware)

**This task is manual and cannot be automated.** Run it before marking the feature done.

- [ ] **Step 1: Verify the MCU is connected and flashed**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
python -m mklink discover
python -m mklink project-info
```

Expected: shows the configured COM port, MCU type `gd32f4`, and the existing flash.

- [ ] **Step 2: Start the server in the foreground**

```bash
python tools/lcd_viewer/server.py -v
```

Expected output (stderr):
```
2026-06-07 ... INFO  lcd_viewer.ws_server listening on ws://127.0.0.1:8765/ws
[lcd_viewer] Open http://127.0.0.1:8765/ in a browser
[lcd_viewer] Default period: 50 ms
```

- [ ] **Step 3: Open the browser and verify the live frame**

Open `http://127.0.0.1:8765/`. Verify:
- [ ] Status dot is green (●).
- [ ] Status text says `running`.
- [ ] FPS shows ~18-20.
- [ ] Canvas is showing the current LCD content (e.g., if the device is in `DOTS` test mode, you should see animated dots).

- [ ] **Step 4: Verify controls**

- [ ] Click `⏸ 暂停`. The button text changes to `▶ 继续`. The FPS counter keeps incrementing (server is still dumping), but the canvas is frozen.
- [ ] Click `▶ 继续`. The canvas resumes updating.
- [ ] Change the period input from 50 to 100. The FPS should drop to ~10 within ~2 seconds. Change back to 50.
- [ ] Click `💾 保存当前帧`. A `.png` file downloads. Open it — it should be a 240×160 black-and-white image of the current LCD state.

- [ ] **Step 5: Verify cleanup**

- [ ] Close the browser tab. The server log should print `last client gone — stopping dumper` and `subprocess terminated`.
- [ ] Press `Ctrl+C` in the server terminal. It should print `shutting down...` and exit cleanly.

- [ ] **Step 6: Document the manual test result**

Open `tools/lcd_viewer/README.md` and append a `## Manual smoke test (2026-06-07)` section with date, port, FPS observed, and any issues found.

- [ ] **Step 7: Commit any final tweaks**

```bash
cd "D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI"
git add -A
git commit -m "docs(lcd_viewer): record manual smoke test result"
```

---

## Self-Review Checklist (run before declaring done)

- [ ] All 23 unit + integration tests pass.
- [ ] `python tools/lcd_viewer/server.py` starts without error.
- [ ] Browser opens `http://127.0.0.1:8765/` and renders the canvas.
- [ ] Pause / period / save controls all work.
- [ ] Closing the browser stops the subprocess.
- [ ] No secrets, no leaked COM ports, no hardcoded paths outside `config.py`.
- [ ] No imports go `web/` → Python (frontend stays standalone).
- [ ] All file paths in this plan exist in the repo.

## Spec Coverage Matrix

| Spec section | Implemented in |
|--------------|----------------|
| §Architecture (3 layers) | Tasks 1–7 |
| §File Structure | All tasks |
| §Key Decisions (10) | Tasks 1 (config), 3 (dumper), 4 (ws), 7 (frontend) |
| §Data Flow (start/connect/disconnect) | Tasks 3, 4 |
| §WebSocket Protocol (binary+JSON) | Tasks 4, 7 |
| §Save PNG flow | Tasks 4, 7 |
| §State Machine (5 states) | Task 3 (Dumper) + Task 4 (WSServer wiring) |
| §Error Handling (9 cases) | Task 3 (Dumper), Task 4 (status broadcast) |
| §Core Algorithms (Frame parse, Dumper class) | Tasks 2, 3 |
| §Frontend Sketch (240×160, pixelated, top bar, bottom controls) | Task 6, 7 |
| §Initial Loading UX | Task 7 (status: "等待首帧…") |
| §Testing Strategy (unit + integration + manual) | Tasks 1–9 (unit/integration), Task 10 (manual) |
| §Out of Scope | All tasks respect read-only, single-region, localhost |
