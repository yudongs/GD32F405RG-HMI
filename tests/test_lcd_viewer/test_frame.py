"""Tests for tools.lcd_viewer.frame — JSON line -> Block -> Frame (via accumulator).

Mirrors the real `mklink dump-memory --json` output: each line is one B1 chunk
(block_index 0..block_count-1) of a sample. Hex is space-separated uppercase.
"""
import json

import pytest

from tools.lcd_viewer import config
from tools.lcd_viewer.frame import (
    Block,
    Frame,
    FrameAccumulator,
    parse_dump_memory_block,
)


def _mkline(
    block_index: int,
    block_count: int,
    ts_us: int = 1,
    size: int = 2048,
    fill: int = 0xAB,
) -> str:
    """Build a single mklink-format dump-memory JSON line (one B1 chunk)."""
    obj = {
        "timestamp_us": ts_us,
        "format": "B1",
        "flags": 0,
        "regions": [{
            "index": 0,
            "address": "0x20001BA4",
            "size": size,
            "hex": " ".join(f"{fill:02X}" for _ in range(size)),
        }],
        "total_size": 4800,
        "block_size": 2048,
        "block_index": block_index,
        "block_count": block_count,
        "block_crc_ok": True,
    }
    return json.dumps(obj)


# ---------- parse_dump_memory_block ----------


def test_parse_normal_block():
    line = _mkline(block_index=0, block_count=3, ts_us=42, fill=0xAB)
    block = parse_dump_memory_block(line)
    assert isinstance(block, Block)
    assert block.ts_us == 42
    assert block.block_index == 0
    assert block.block_count == 3
    assert block.crc_ok is True
    assert len(block.payload) == 2048
    assert block.payload[:8] == b"\xab" * 8


def test_parse_handles_hex_with_spaces():
    line = _mkline(block_index=1, block_count=3, fill=0xCD)
    block = parse_dump_memory_block(line)
    # 2048 bytes of 0xCD.
    assert block.payload == b"\xcd" * 2048


def test_parse_short_block_uses_region_size():
    """Final block may be shorter than block_size (e.g. 704 for 4800/2048 split)."""
    line = _mkline(block_index=2, block_count=3, size=704, fill=0x55)
    block = parse_dump_memory_block(line)
    assert len(block.payload) == 704
    assert block.payload[0] == 0x55


def test_parse_rejects_malformed_json():
    with pytest.raises(json.JSONDecodeError):
        parse_dump_memory_block("not json at all")


def test_parse_defaults_crc_ok_to_true():
    """If `block_crc_ok` is missing, assume True (don't fail the reader)."""
    obj = {
        "timestamp_us": 1,
        "format": "B1",
        "flags": 0,
        "regions": [{
            "index": 0,
            "address": "0x20001BA4",
            "size": 8,
            "hex": "AA BB CC DD EE FF 00 11",
        }],
        "total_size": 8,
        "block_size": 8,
        "block_index": 0,
        "block_count": 1,
        # no block_crc_ok
    }
    block = parse_dump_memory_block(json.dumps(obj))
    assert block.crc_ok is True


def test_parse_reports_crc_failure():
    line = _mkline(block_index=0, block_count=1).replace('"block_crc_ok": true',
                                                        '"block_crc_ok": false')
    block = parse_dump_memory_block(line)
    assert block.crc_ok is False


# ---------- FrameAccumulator ----------


def test_accumulator_assembles_three_blocks_into_one_frame():
    """Each block of a real mklink sample has its own timestamp_us."""
    acc = FrameAccumulator()
    f0 = acc.feed(parse_dump_memory_block(_mkline(0, 3, ts_us=100, fill=0x11)))
    f1 = acc.feed(parse_dump_memory_block(_mkline(1, 3, ts_us=101, fill=0x22)))
    f2 = acc.feed(parse_dump_memory_block(_mkline(2, 3, ts_us=102, size=704, fill=0x33)))
    assert f0 is None
    assert f1 is None
    assert isinstance(f2, Frame)
    # ts_us of the assembled Frame = ts of the first block of the sample
    assert f2.ts_us == 100
    assert len(f2.payload) == config.FB_SIZE  # 4800
    # Each block is contiguous: first 2048 = 0x11, next 2048 = 0x22, last 704 = 0x33
    assert f2.payload[0] == 0x11
    assert f2.payload[2047] == 0x11
    assert f2.payload[2048] == 0x22
    assert f2.payload[4095] == 0x22
    assert f2.payload[4096] == 0x33
    assert f2.payload[-1] == 0x33


def test_accumulator_returns_none_on_partial_sample_one_block():
    acc = FrameAccumulator()
    assert acc.feed(parse_dump_memory_block(_mkline(0, 3, fill=0xAA))) is None


def test_accumulator_returns_none_on_partial_sample_two_blocks():
    acc = FrameAccumulator()
    acc.feed(parse_dump_memory_block(_mkline(0, 3, fill=0xAA)))
    assert acc.feed(parse_dump_memory_block(_mkline(1, 3, fill=0xBB))) is None


def test_accumulator_returns_none_on_wrong_size_payload():
    """If concatenated blocks don't sum to FB_SIZE, drop the sample."""
    acc = FrameAccumulator()
    # block_size 100, block_count 2 → 200 bytes, not 4800
    f0 = acc.feed(parse_dump_memory_block(_mkline(0, 2, size=100, fill=0x11)))
    f1 = acc.feed(parse_dump_memory_block(_mkline(1, 2, size=100, fill=0x22)))
    assert f0 is None
    assert f1 is None  # size mismatch → no Frame


def test_accumulator_resets_on_new_sample_boundary():
    """If a new sample's block 0 arrives mid-sample, drop the old one."""
    acc = FrameAccumulator()
    acc.feed(parse_dump_memory_block(_mkline(0, 3, ts_us=100, fill=0x11)))
    # New sample starts before the old one finished
    acc.feed(parse_dump_memory_block(_mkline(0, 3, ts_us=200, fill=0xAA)))
    acc.feed(parse_dump_memory_block(_mkline(1, 3, ts_us=201, fill=0xBB)))
    frame = acc.feed(parse_dump_memory_block(_mkline(2, 3, ts_us=202, size=704, fill=0xCC)))
    assert isinstance(frame, Frame)
    assert frame.ts_us == 200
    # All bytes should be from the new sample
    assert frame.payload[0] == 0xAA
    assert frame.payload[2048] == 0xBB
    assert frame.payload[4096] == 0xCC


def test_accumulator_handles_single_block_sample():
    acc = FrameAccumulator()
    frame = acc.feed(parse_dump_memory_block(_mkline(0, 1, size=4800, fill=0x99)))
    assert isinstance(frame, Frame)
    assert frame.payload == b"\x99" * 4800


# ---------- frozen dataclasses ----------


def test_block_is_frozen():
    b = Block(ts_us=1, block_index=0, block_count=1, payload=b"", crc_ok=True)
    with pytest.raises(Exception):
        b.ts_us = 2  # type: ignore[misc]


def test_frame_is_frozen():
    f = Frame(ts_us=1, payload=b"\x00" * config.FB_SIZE)
    with pytest.raises(Exception):
        f.ts_us = 2  # type: ignore[misc]
