"""VTFP v1 header pack/unpack.

Mirrors `vtfp-protocol/c-types/vtfp_header.h` byte-for-byte. Field order,
sizes, and offsets must match the C `_Static_assert(sizeof == 44)`.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from typing import Optional

from .constants import VTFP_MAGIC, VTFP_VERSION

HEADER_SIZE = 44

# struct format: 12 fields, total 44 bytes, little-endian, no padding
# <  = little-endian
# I  = uint32  (4)  magic
# H  = uint16  (2)  version
# H  = uint16  (2)  features
# I  = uint32  (4)  command
# I  = uint32  (4)  result
# I  = uint32  (4)  seq
# I  = uint32  (4)  param
# I  = uint32  (4)  data_addr
# I  = uint32  (4)  data_len
# I  = uint32  (4)  flags
# I  = uint32  (4)  checksum
# I  = uint32  (4)  reserved
# Total: 4+2+2+4+4+4+4+4+4+4+4+4 = 44
_HEADER_STRUCT = struct.Struct("<IHHIIIIIIIII")


@dataclass
class VTFHeader:
    """VTFP v1 control header (44 bytes)."""
    command:    int = 0
    result:     int = 0
    seq:        int = 0
    param:      int = 0
    data_addr:  int = 0
    data_len:   int = 0
    flags:      int = 0
    features:   int = 0
    checksum:   int = 0

    def pack(self) -> bytes:
        """Serialize to 44 bytes, including magic/version prefix and checksum placeholder."""
        return _HEADER_STRUCT.pack(
            VTFP_MAGIC,
            VTFP_VERSION,
            self.features & 0xFFFF,
            self.command & 0xFFFFFFFF,
            self.result & 0xFFFFFFFF,
            self.seq & 0xFFFFFFFF,
            self.param & 0xFFFFFFFF,
            self.data_addr & 0xFFFFFFFF,
            self.data_len & 0xFFFFFFFF,
            self.flags & 0xFFFFFFFF,
            self.checksum & 0xFFFFFFFF,
            0,  # reserved
        )


def pack_header(
    command: int = 0,
    result: int = 0,
    seq: int = 0,
    param: int = 0,
    data_addr: int = 0,
    data_len: int = 0,
    flags: int = 0,
    features: int = 0,
    checksum: int = 0,
) -> bytes:
    """Pack a header into 44 bytes. Convenience wrapper around VTFHeader."""
    return VTFHeader(
        command=command, result=result, seq=seq, param=param,
        data_addr=data_addr, data_len=data_len, flags=flags,
        features=features, checksum=checksum,
    ).pack()


def unpack_header(blob: bytes) -> VTFHeader:
    """Unpack 44 bytes into a VTFHeader. Raises ValueError on size mismatch."""
    if len(blob) != HEADER_SIZE:
        raise ValueError(
            f"VTFP header must be exactly {HEADER_SIZE} bytes, got {len(blob)}"
        )
    (
        magic, version, features, command, result, seq, param,
        data_addr, data_len, flags, checksum, reserved,
    ) = _HEADER_STRUCT.unpack(blob)
    if magic != VTFP_MAGIC:
        raise ValueError(f"Invalid magic: 0x{magic:08X}, expected 0x{VTFP_MAGIC:08X}")
    if version != VTFP_VERSION:
        raise ValueError(f"Unsupported version: 0x{version:04X}, expected 0x{VTFP_VERSION:04X}")
    if reserved != 0:
        raise ValueError(f"Reserved field must be 0, got 0x{reserved:08X}")
    return VTFHeader(
        command=command, result=result, seq=seq, param=param,
        data_addr=data_addr, data_len=data_len, flags=flags,
        features=features, checksum=checksum,
    )
