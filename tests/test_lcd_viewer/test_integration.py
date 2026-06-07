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

                # Collect 3 binary frames. The server emits JSON-encoded control
                # messages (hello, status) as binary frames too, so distinguish
                # by length: a real framebuffer payload is exactly FB_SIZE bytes.
                received = []
                deadline = asyncio.get_event_loop().time() + 5.0
                while len(received) < 3 and asyncio.get_event_loop().time() < deadline:
                    raw = await asyncio.wait_for(ws.recv(), 2.0)
                    assert isinstance(raw, bytes), f"unexpected non-bytes frame: {raw!r}"
                    if len(raw) != config.FB_SIZE:
                        # JSON control / status message — ignore.
                        continue
                    received.append(raw)
                assert len(received) == 3, f"only received {len(received)} binary frames"

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
