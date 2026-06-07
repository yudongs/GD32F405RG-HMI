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
