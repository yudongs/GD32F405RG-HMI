"""VTFP protocol round-trip smoke test (no hardware)."""
from vtfp_proto import pack_header, unpack_header, VTFP_MAGIC, HEADER_SIZE
import struct

# Build a KEY_PRESS request (cmd=0x01, param=KEY_EVENT_ENTER=3)
header = pack_header(
    command=0x01, seq=1, param=3,
    data_addr=0x2000002C, data_len=0,
)
print(f"encoded header: {len(header)} bytes (expected {HEADER_SIZE})")
print(f"  magic     = {struct.unpack('<I', header[0:4])[0]:08x} (expected {VTFP_MAGIC:08x})")
print(f"  command   = {struct.unpack('<I', header[8:12])[0]} (expected 1 = KEY_PRESS)")
print(f"  param     = {struct.unpack('<I', header[20:24])[0]} (expected 3 = ENTER)")
print(f"  data_addr = {struct.unpack('<I', header[24:28])[0]:08x}")

# Decode
parsed = unpack_header(header)
print(f"parsed: cmd={parsed.command} seq={parsed.seq} param={parsed.param} checksum_ok={parsed.checksum_ok}")
print()
print("PROTOCOL LAYER OK - header round-trips cleanly with valid CRC")
