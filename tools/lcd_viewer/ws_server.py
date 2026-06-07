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
import io
import json
import logging
import pathlib
import struct
import threading
from typing import Any, Protocol

import websockets
from websockets.asyncio.server import ServerConnection, serve
from websockets.datastructures import Headers
from websockets.http11 import Request, Response

from tools.lcd_viewer import config

log = logging.getLogger("lcd_viewer.ws_server")


class DumperLike(Protocol):
    """Anything that quacks like a Dumper — kept loose for testing."""
    state: Any
    latest_frame: Any

    async def start(self, period_ms: int) -> None: ...
    async def stop(self) -> None: ...
    def set_state_callback(self, cb) -> None: ...
    def set_new_frame_callback(self, cb) -> None: ...


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
        # Event-driven broadcast: dumper sets this when a new frame is
        # published, broadcast loop awaits it in a worker thread.
        self._new_frame_event = threading.Event()
        # Static file directory.
        self._web_dir = pathlib.Path(__file__).parent / "web"
        # dumper → ws_server state broadcast.
        self._dumper.set_state_callback(self._on_dumper_state)
        # dumper → ws_server new-frame signal.
        self._dumper.set_new_frame_callback(self._on_new_frame)
        # Default period for first start.
        self._period_ms: int = config.DEFAULT_PERIOD_MS

    @property
    def client_count(self) -> int:
        return self._client_count

    # ----- lifecycle -----

    async def start(self, host: str = config.HOST, port: int = config.PORT) -> None:
        # process_request runs BEFORE the WebSocket handshake. If it returns
        # None, the WS handshake proceeds. If it returns a Response, the WS
        # handshake is skipped and that HTTP response is sent instead.
        # We use it to route plain HTTP GETs to the static-file handler
        # and let /ws continue through the WebSocket handler.
        self._ws_server = await serve(
            self._handler, host, port,
            process_request=self._process_request,
            ping_interval=20, ping_timeout=20,
        )
        log.info("listening on ws://%s:%d%s", host, port, config.WS_PATH)

    async def stop(self) -> None:
        if self._ws_server is not None:
            self._ws_server.close()
            await self._ws_server.wait_closed()
            self._ws_server = None
        # Wake the broadcast loop so it exits cleanly.
        self._new_frame_event.set()
        if self._state_task is not None and not self._state_task.done():
            try:
                await asyncio.wait_for(self._state_task, timeout=2.0)
            except (asyncio.TimeoutError, asyncio.CancelledError, Exception):
                self._state_task.cancel()
                try:
                    await self._state_task
                except (asyncio.CancelledError, Exception):
                    pass
        self._state_task = None

    async def serve_forever(self) -> None:
        if self._ws_server is not None:
            await self._ws_server.wait_closed()

    # ----- connection handler -----

    async def _handler(self, connection: ServerConnection) -> None:
        # process_request already routed non-WS paths to static files. If
        # we reach the handler, this is a real WebSocket upgrade on /ws.
        log.info("client connected: %s %s", connection.remote_address, connection.request.path)
        await self._ws_loop(connection)

    # ----- HTTP static files (process_request) -----

    def _process_request(
        self, connection: ServerConnection, request: Request
    ) -> Response | None:
        """Intercepts every connection before the WebSocket handshake.

        - /ws (with or without query string) → return None → WS handshake proceeds.
        - Anything else → serve a static file from self._web_dir, return Response.
        """
        path = request.path
        if path == config.WS_PATH or path.startswith(config.WS_PATH + "?"):
            return None  # proceed with WebSocket handshake
        return self._build_static_response(path)

    def _build_static_response(self, path: str) -> Response:
        # Strip query string first, then map "/" → index.html.
        path_no_query = path.split("?", 1)[0]
        if path_no_query in ("/", ""):
            rel = "index.html"
        else:
            rel = path_no_query.lstrip("/")
        # Prevent path traversal: resolved target must live inside _web_dir.
        web_dir = self._web_dir.resolve()
        target = (self._web_dir / rel).resolve()
        if web_dir not in target.parents and target != web_dir:
            return self._http_response(403, b"forbidden", "text/plain")
        if not target.is_file():
            return self._http_response(404, b"not found", "text/plain")
        ext = target.suffix.lower()
        ctype = {
            ".html": "text/html; charset=utf-8",
            ".js": "application/javascript; charset=utf-8",
            ".css": "text/css; charset=utf-8",
            ".png": "image/png",
            ".svg": "image/svg+xml",
            ".ico": "image/x-icon",
            ".json": "application/json; charset=utf-8",
        }.get(ext, "application/octet-stream")
        body = target.read_bytes()
        return self._http_response(200, body, ctype)

    @staticmethod
    def _http_response(status: int, body: bytes, ctype: str) -> Response:
        reason = {200: "OK", 403: "Forbidden", 404: "Not Found"}.get(status, "OK")
        # Headers takes list-of-pairs; pass the canonical HTTP header names.
        headers = Headers([
            ("Content-Type", ctype),
            ("Content-Length", str(len(body))),
            ("Connection", "close"),
        ])
        return Response(status, reason, headers, body)

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
            img = Image.frombytes("1", (config.FB_WIDTH, config.FB_HEIGHT), frame.payload)
            buf = io.BytesIO()
            img.save(buf, format="PNG")
            png_bytes = buf.getvalue()
            # Frame: [4B LE uint32 length][PNG bytes]
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
                # Wake the broadcast loop so it notices client_count == 0
                # and exits, instead of blocking forever in wait().
                self._new_frame_event.set()

    async def _broadcast_loop(self) -> None:
        """Push frames to clients as soon as the dumper publishes them.

        Event-driven: the dumper's reader sets ``_new_frame_event`` after
        updating ``latest_frame``. We wait on that event in a worker thread
        (so the asyncio event loop is never blocked), then drain the event,
        snapshot the latest frame, and ship it to every connected client.

        Drain-after-wait ordering guarantees we never lose a frame: if
        frames arrive while we are sending the previous one, the event is
        set again and the next ``wait()`` returns immediately.
        """
        last_sent_ts: int = -1
        loop = asyncio.get_running_loop()
        while self._client_count > 0:
            # Block on the dumper's signal without blocking the event loop.
            await loop.run_in_executor(None, self._new_frame_event.wait)
            self._new_frame_event.clear()
            frame = self._dumper.latest_frame
            if frame is None or frame.ts_us == last_sent_ts:
                continue
            last_sent_ts = frame.ts_us
            # Snapshot the client set; copy to avoid mutation during send.
            targets = list(self._clients)
            for c in targets:
                try:
                    await c.send(frame.payload)
                except Exception:
                    pass  # client will be cleaned up on next event

    def _on_new_frame(self) -> None:
        """Dumper callback: wake the broadcast loop. Runs in dumper thread."""
        self._new_frame_event.set()

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
