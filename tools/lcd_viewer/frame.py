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
