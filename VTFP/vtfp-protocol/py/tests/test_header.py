"""Tests for header pack/unpack."""

import pytest
from vtfp_proto import (
    VTFP_MAGIC, VTFP_VERSION,
    pack_header, unpack_header, VTFHeader, HEADER_SIZE,
)


class TestHeaderSize:
    def test_header_size_constant(self):
        assert HEADER_SIZE == 44

    def test_pack_returns_44_bytes(self):
        assert len(pack_header()) == 44

    def test_roundtrip_preserves_size(self):
        h = pack_header(command=0x42)
        assert len(h) == 44
        unpack_header(h)  # must not raise


class TestMagicAndVersion:
    def test_packed_header_starts_with_magic(self):
        blob = pack_header()
        # First 4 bytes = 'V','T','F','P' little-endian
        assert blob[:4] == b"VTFP"

    def test_version_field_at_offset_4(self):
        blob = pack_header()
        # version = 0x0001, little-endian = b"\x01\x00"
        assert blob[4:6] == b"\x01\x00"


class TestFieldRoundTrip:
    def test_command_roundtrip(self):
        h = unpack_header(pack_header(command=0xDEADBEEF))
        assert h.command == 0xDEADBEEF

    def test_result_roundtrip(self):
        h = unpack_header(pack_header(result=0xCAFEBABE))
        assert h.result == 0xCAFEBABE

    def test_seq_roundtrip(self):
        h = unpack_header(pack_header(seq=12345))
        assert h.seq == 12345

    def test_param_roundtrip(self):
        h = unpack_header(pack_header(param=0x12345678))
        assert h.param == 0x12345678

    def test_data_addr_roundtrip(self):
        h = unpack_header(pack_header(data_addr=0x20010000))
        assert h.data_addr == 0x20010000

    def test_data_len_roundtrip(self):
        h = unpack_header(pack_header(data_len=512))
        assert h.data_len == 512

    def test_flags_roundtrip(self):
        h = unpack_header(pack_header(flags=0xAA55AA55))
        assert h.flags == 0xAA55AA55

    def test_features_roundtrip(self):
        h = unpack_header(pack_header(features=0x0007))
        assert h.features == 0x0007

    def test_checksum_roundtrip(self):
        h = unpack_header(pack_header(checksum=0xDEADBEEF))
        assert h.checksum == 0xDEADBEEF


class TestFieldLayout:
    """Confirm each field is at the byte offset that C asserts on."""

    def _read(self, blob, offset, size, fmt):
        import struct
        return struct.unpack_from(fmt, blob, offset)[0]

    def test_magic_at_offset_0(self):
        blob = pack_header()
        assert self._read(blob, 0, 4, "<I") == VTFP_MAGIC

    def test_version_at_offset_4(self):
        blob = pack_header()
        assert self._read(blob, 4, 2, "<H") == VTFP_VERSION

    def test_command_at_offset_8(self):
        blob = pack_header(command=0xABCD1234)
        assert self._read(blob, 8, 4, "<I") == 0xABCD1234

    def test_result_at_offset_12(self):
        blob = pack_header(result=0x11223344)
        assert self._read(blob, 12, 4, "<I") == 0x11223344

    def test_seq_at_offset_16(self):
        blob = pack_header(seq=0x55667788)
        assert self._read(blob, 16, 4, "<I") == 0x55667788

    def test_param_at_offset_20(self):
        blob = pack_header(param=0x99AABBCC)
        assert self._read(blob, 20, 4, "<I") == 0x99AABBCC

    def test_data_addr_at_offset_24(self):
        blob = pack_header(data_addr=0x20020000)
        assert self._read(blob, 24, 4, "<I") == 0x20020000

    def test_data_len_at_offset_28(self):
        blob = pack_header(data_len=256)
        assert self._read(blob, 28, 4, "<I") == 256

    def test_flags_at_offset_32(self):
        blob = pack_header(flags=0xF0F0F0F0)
        assert self._read(blob, 32, 4, "<I") == 0xF0F0F0F0

    def test_checksum_at_offset_36(self):
        blob = pack_header(checksum=0x12345678)
        assert self._read(blob, 36, 4, "<I") == 0x12345678

    def test_reserved_at_offset_40_is_zero(self):
        blob = pack_header()
        assert self._read(blob, 40, 4, "<I") == 0


class TestErrorPaths:
    def test_short_blob_raises(self):
        with pytest.raises(ValueError, match="must be exactly 44 bytes"):
            unpack_header(b"\x00" * 43)

    def test_long_blob_raises(self):
        with pytest.raises(ValueError, match="must be exactly 44 bytes"):
            unpack_header(b"\x00" * 45)

    def test_bad_magic_raises(self):
        blob = bytearray(pack_header())
        blob[0] = 0  # corrupt magic
        with pytest.raises(ValueError, match="Invalid magic"):
            unpack_header(bytes(blob))

    def test_bad_version_raises(self):
        blob = bytearray(pack_header())
        blob[4] = 0x99  # corrupt version
        with pytest.raises(ValueError, match="Unsupported version"):
            unpack_header(bytes(blob))

    def test_nonzero_reserved_raises(self):
        blob = bytearray(pack_header())
        blob[40] = 0x01  # corrupt reserved
        with pytest.raises(ValueError, match="Reserved field"):
            unpack_header(bytes(blob))


class TestEndianness:
    def test_packed_header_is_little_endian(self):
        # If we set a high-bit pattern, the first byte at that field's
        # offset should be the low byte of the value.
        blob = pack_header(command=0x12345678)
        assert blob[8] == 0x78
        assert blob[9] == 0x56
        assert blob[10] == 0x34
        assert blob[11] == 0x12
