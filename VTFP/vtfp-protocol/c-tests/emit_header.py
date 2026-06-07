#!/usr/bin/env python3
"""Emit a 44-byte VTFP header blob to stdout for test_cross_lang.c to read.

Usage: python emit_header.py <out.bin>
"""
import sys
from pathlib import Path

# Ensure py/src is on path
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "py" / "src"))

from vtfp_proto import (
    pack_header, crc32, VTFP_MAGIC, VTFP_VERSION,
)

# Build a known header
payload = bytearray(pack_header(
    command=0x42,
    result=0x07,
    seq=0x12345678,
    param=0xDEADBEEF,
    data_addr=0x20010000,
    data_len=0x100,
    flags=0xAA55AA55,
    features=0x0003,  # LCD + RTT
    checksum=0,  # computed below
))

# CRC over bytes [0..35] (excluding checksum+reserved)
crc = crc32(bytes(payload[0:36]))
struct_pack = bytearray(payload)
struct_pack[36:40] = crc.to_bytes(4, "little")

# Write to file
out_path = sys.argv[1] if len(sys.argv) > 1 else "header.bin"
Path(out_path).write_bytes(bytes(struct_pack))
print(f"Wrote {len(struct_pack)} bytes to {out_path}", file=sys.stderr)
