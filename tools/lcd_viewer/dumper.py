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
        # Try to close stdout first to unblock the reader thread.
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
        `loop.run_in_executor(None, q.get)` with short blocking so it
        remains responsive to cancellation.
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
