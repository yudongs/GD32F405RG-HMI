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
