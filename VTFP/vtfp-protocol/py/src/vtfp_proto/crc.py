"""VTFP v1 CRC-32 (IEEE 802.3).

Must match `vtfp-protocol/c-tests/test_crc.c` byte-for-byte. The C
implementation is a byte-at-a-time algorithm using a 32-bit register; the
Python version mirrors this for cross-language testability.

Polynomial: 0xEDB88320 (reflected form of 0x04C11DB7)
Init:       0xFFFFFFFF
RefIn/Out:  True
XorOut:     0xFFFFFFFF
Check:      CRC32("123456789") == 0xCBF43926
"""

_POLY = 0xEDB88320


def _update(crc: int, byte: int) -> int:
    crc ^= byte
    for _ in range(8):
        mask = -(crc & 1)  # arithmetic shift: all-1s if low bit set, else all-0s
        crc = (crc >> 1) ^ (_POLY & mask)
    return crc & 0xFFFFFFFF


def crc32(data: bytes | bytearray | memoryview) -> int:
    """Compute CRC-32 over `data`. Returns 32-bit unsigned int."""
    crc = 0xFFFFFFFF
    for byte in data:
        crc = _update(crc, byte)
    return crc ^ 0xFFFFFFFF
