"""Parse JSON lines from `python -m mklink dump-memory --json` and reconstruct
full framebuffer samples from B1-chunked output.

Real protocol (verified against mklink 0.1.0 on hardware):
- Each stdout line is ONE B1 chunk of a sample
- For a 4800-byte region with 2048-byte blocks, dump-memory emits 3 lines
  (block_index 0, 1, 2) per sample
- A full sample = `block_count` consecutive lines with matching
  `timestamp_us` and `block_crc_ok: true`, then concatenated in `index` order
- Region `hex` is a space-separated uppercase hex string (e.g., "00 AB CD ...")

Public API:
- `Block` dataclass: one chunk
- `parse_dump_memory_block(line: str) -> Block`: parse one JSON line
- `FrameAccumulator`: stateful collector that emits `Frame` objects when a
  complete sample is assembled
"""
from __future__ import annotations

import json
from dataclasses import dataclass, field

from tools.lcd_viewer import config


@dataclass(frozen=True)
class Frame:
    """A complete framebuffer sample (all blocks assembled).

    `ts_us` is the device's microsecond timestamp at the moment the snapshot
    was taken. `payload` is exactly `config.FB_SIZE` raw bytes.
    """
    ts_us: int
    payload: bytes


@dataclass(frozen=True)
class Block:
    """One B1 chunk from a single dump-memory JSON line."""
    ts_us: int
    block_index: int
    block_count: int
    payload: bytes
    crc_ok: bool


def parse_dump_memory_block(line: str) -> Block:
    """Parse one stdout line from `dump-memory --json` into a Block.

    Raises json.JSONDecodeError if the line is not valid JSON.
    Raises KeyError if required fields are missing.
    """
    obj = json.loads(line)
    hex_str = obj["regions"][0]["hex"]
    payload = bytes.fromhex(hex_str.replace(" ", ""))
    return Block(
        ts_us=obj["timestamp_us"],
        block_index=obj["block_index"],
        block_count=obj["block_count"],
        payload=payload,
        crc_ok=obj.get("block_crc_ok", True),
    )


@dataclass
class FrameAccumulator:
    """Reconstructs Frame objects from a stream of B1 blocks.

    Drop in place of the dumper's old behavior:
        acc = FrameAccumulator()
        for line in proc.stdout:
            block = parse_dump_memory_block(line)
            frame = acc.feed(block)
            if frame is not None:
                # complete Frame available
                ...
    """
    _blocks: dict[int, bytes] = field(default_factory=dict)
    _ts_us: int = 0
    _block_count: int = 0
    _expected_crc_ok: bool = True

    def feed(self, block: Block) -> Frame | None:
        """Add a block. Returns a Frame when a complete sample is assembled,
        else None. Returns None on a malformed sample (missing/extra block).

        Sample boundary detection: a sample starts when ``block_index == 0``
        OR when the ``block_count`` changes. ``timestamp_us`` is per-block
        (each block of a sample has its own ts, not a shared one), so we
        do NOT use ts equality to detect boundaries.
        """
        if self._block_count == 0 or block.block_index == 0:
            # First block of a new sample (or recovering from a boundary)
            if self._block_count != 0 and block.block_index == 0:
                # New sample started; previous one was incomplete — drop it.
                self._blocks.clear()
            self._block_count = block.block_count
            self._ts_us = block.ts_us
            self._expected_crc_ok = block.crc_ok
        elif block.block_count != self._block_count:
            # block_count changed mid-sample (e.g. CRC-failed retry) — reset.
            self._blocks.clear()
            self._block_count = block.block_count
            self._ts_us = block.ts_us
            self._expected_crc_ok = block.crc_ok

        self._blocks[block.block_index] = block.payload

        if block.block_index == self._block_count - 1:
            # Last block received — try to assemble
            if len(self._blocks) != self._block_count:
                # Missing intermediate blocks
                return None
            payload = b"".join(self._blocks[i] for i in range(self._block_count))
            self._blocks.clear()
            self._block_count = 0
            if len(payload) != config.FB_SIZE:
                return None  # size mismatch — skip
            return Frame(ts_us=self._ts_us, payload=payload)
        return None
