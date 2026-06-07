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
