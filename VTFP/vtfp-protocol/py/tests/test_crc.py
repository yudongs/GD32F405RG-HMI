"""Tests for CRC-32 — must pass the same known-answer values as the C test."""

import pytest
from vtfp_proto import crc32


class TestKnownAnswers:
    """Same values as c-tests/test_crc.c."""

    def test_check_value(self):
        # Standard CRC-32 check value: CRC of "123456789"
        assert crc32(b"123456789") == 0xCBF43926

    def test_empty_input(self):
        assert crc32(b"") == 0x00000000

    def test_single_byte_a(self):
        assert crc32(b"a") == 0xE8B7BE43


class TestDeterminism:
    def test_same_input_same_output(self):
        msg = b"VTFP v1"
        assert crc32(msg) == crc32(msg)

    def test_chunked_equals_whole(self):
        msg = b"Hello, World!"
        whole = crc32(msg)
        # crc32() applies the final XOR; a chunked API tracks the raw
        # register, so undo the public XOR before continuing the chain.
        chunked = crc32(msg[:5]) ^ 0xFFFFFFFF
        for b in msg[5:]:
            chunked = _update_inline(chunked, b)
        chunked ^= 0xFFFFFFFF
        # This test guards against accidentally adding an extra XOR at
        # the end of the public function (or omitting it).
        assert whole == chunked


def _update_inline(crc, byte):
    """Mirror of the C update — only for the chunked-vs-whole test."""
    crc ^= byte
    for _ in range(8):
        mask = -(crc & 1)
        crc = (crc >> 1) ^ (0xEDB88320 & mask)
    return crc & 0xFFFFFFFF


class TestByteRange:
    @pytest.mark.parametrize("b", list(range(256)))
    def test_all_byte_values_deterministic(self, b):
        # Process the same byte twice; result should be stable.
        c1 = crc32(bytes([b]))
        c2 = crc32(bytes([b]))
        assert c1 == c2

    def test_all_byte_values_distinct(self):
        # Each of the 256 possible byte values should produce a distinct
        # CRC of (single byte), because CRC-32 with init=xorout=0 is
        # effectively a bijection on the 256-element input space.
        seen = set()
        for b in range(256):
            seen.add(crc32(bytes([b])))
        assert len(seen) == 256


class TestCrossLanguage:
    """The C test_crc.c computes the same known answers; verify Python
    matches them bit-for-bit. If this test ever fails, the wire format
    between MCU and PC is broken."""

    def test_c_known_answer_1(self):
        assert crc32(b"123456789") == 0xCBF43926

    def test_c_known_answer_2(self):
        assert crc32(b"") == 0x00000000

    def test_c_known_answer_3(self):
        assert crc32(b"a") == 0xE8B7BE43

    def test_c_determinism_value_vtfp_v1(self):
        """M1-T5 C test reported CRC of "VTFP v1" as 0x463E73F3.
        If this test fails, the wire format between C and Python is broken."""
        assert crc32(b"VTFP v1") == 0x463E73F3
