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
