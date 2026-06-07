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

    def set_new_frame_callback(self, cb):
        # No-op for the fake — the test harness drives frames via push_frame().
        pass

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
        async with websockets.connect("ws://127.0.0.1:18766/ws") as ws:
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


# ----- HTTP static-file tests (regression for the 426 Upgrade Required bug) -----

async def _http_get(host: str, port: int, path: str, timeout: float = 2.0) -> tuple[int, dict, bytes]:
    """Async HTTP GET that cooperates with the running event loop.

    Returns (status, headers, body). Uses a fresh TCP connection per call.
    """
    reader, writer = await asyncio.wait_for(
        asyncio.open_connection(host, port), timeout=timeout
    )
    try:
        req = (
            f"GET {path} HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            f"Connection: close\r\n"
            f"\r\n"
        ).encode("ascii")
        writer.write(req)
        await writer.drain()
        # Read until EOF (server uses Connection: close).
        chunks: list[bytes] = []
        while True:
            chunk = await asyncio.wait_for(reader.read(4096), timeout=timeout)
            if not chunk:
                break
            chunks.append(chunk)
        raw = b"".join(chunks)
    finally:
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass
    # Parse status line + headers.
    head, _, body = raw.partition(b"\r\n\r\n")
    status_line, _, header_lines = head.partition(b"\r\n")
    parts = status_line.split(b" ", 2)
    status = int(parts[1]) if len(parts) >= 2 else 0
    headers: dict[str, str] = {}
    for line in header_lines.split(b"\r\n"):
        if b":" in line:
            k, _, v = line.partition(b":")
            headers[k.decode("ascii").strip().lower()] = v.decode("ascii").strip()
    return status, headers, body


async def _start_server_with_web_dir(
    dumper, web_dir, port: int
) -> tuple[WSServer, asyncio.Task]:
    server = WSServer(dumper=dumper)  # type: ignore[arg-type]
    # Replace the static-file dir with our test fixture dir.
    server._web_dir = web_dir  # type: ignore[attr-defined]
    server_task = asyncio.create_task(server.start(host="127.0.0.1", port=port))
    await asyncio.sleep(0.1)  # let the server bind
    return server, server_task


async def _stop_server(server: WSServer, server_task: asyncio.Task) -> None:
    await server.stop()
    server_task.cancel()
    try:
        await server_task
    except (asyncio.CancelledError, Exception):
        pass


@pytest.mark.asyncio
async def test_http_serves_index_html(tmp_path):
    """Plain HTTP GET / must serve web/index.html, not 426 Upgrade Required."""
    (tmp_path / "index.html").write_text("<h1>hi from index</h1>", encoding="utf-8")
    server, server_task = await _start_server_with_web_dir(
        FakeDumper(), tmp_path, 18770
    )
    try:
        status, headers, body = await _http_get("127.0.0.1", 18770, "/")
        assert status == 200, f"expected 200, got {status} (body={body!r})"
        assert b"<h1>hi from index</h1>" in body
        ctype = headers.get("content-type", "")
        assert "text/html" in ctype
    finally:
        await _stop_server(server, server_task)


@pytest.mark.asyncio
async def test_http_returns_404_for_missing_file(tmp_path):
    """Unknown path must return 404, not 426."""
    server, server_task = await _start_server_with_web_dir(
        FakeDumper(), tmp_path, 18771
    )
    try:
        status, _headers, body = await _http_get("127.0.0.1", 18771, "/nonexistent.js")
        assert status == 404, f"expected 404, got {status} (body={body!r})"
    finally:
        await _stop_server(server, server_task)


@pytest.mark.asyncio
async def test_http_serves_app_js(tmp_path):
    """Static file under web/ is served with the right content-type."""
    (tmp_path / "app.js").write_text("//MARKER app.js body", encoding="utf-8")
    server, server_task = await _start_server_with_web_dir(
        FakeDumper(), tmp_path, 18772
    )
    try:
        status, headers, body = await _http_get("127.0.0.1", 18772, "/app.js")
        assert status == 200, f"expected 200, got {status} (body={body!r})"
        assert b"//MARKER app.js body" in body
        ctype = headers.get("content-type", "")
        assert "javascript" in ctype
    finally:
        await _stop_server(server, server_task)


@pytest.mark.asyncio
async def test_http_serves_index_via_query_string(tmp_path):
    """/?foo=bar must still serve index.html (regression for query handling)."""
    (tmp_path / "index.html").write_text("<title>root</title>", encoding="utf-8")
    server, server_task = await _start_server_with_web_dir(
        FakeDumper(), tmp_path, 18773
    )
    try:
        status, _headers, body = await _http_get("127.0.0.1", 18773, "/?foo=bar")
        assert status == 200, f"expected 200, got {status} (body={body!r})"
        assert b"<title>root</title>" in body
    finally:
        await _stop_server(server, server_task)
