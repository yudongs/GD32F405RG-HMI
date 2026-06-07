"""Python-side mirror of the C cross-lang test. Verifies that the
constants and CRC implementation match what C produced."""

import subprocess
import sys
from pathlib import Path

import pytest

# Make package importable
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "src"))

from vtfp_proto import crc32, pack_header, VTFP_MAGIC, VTFP_VERSION, unpack_header


def test_emit_header_runs():
    """emit_header.py should run without error and produce a 44-byte file."""
    emit_script = HERE.parent.parent / "c-tests" / "emit_header.py"
    out = HERE / "_test_header.bin"
    if out.exists():
        out.unlink()

    result = subprocess.run(
        [sys.executable, str(emit_script), str(out)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, f"emit_header.py failed: {result.stderr}"
    assert out.exists(), "header.bin was not created"
    assert out.stat().st_size == 44, f"header.bin should be 44 bytes, got {out.stat().st_size}"

    # Read back and verify it parses
    blob = out.read_bytes()
    hdr = unpack_header(blob)
    assert hdr.command == 0x42
    assert hdr.result == 0x07
    assert hdr.seq == 0x12345678
    assert hdr.param == 0xDEADBEEF
    assert hdr.data_addr == 0x20010000
    assert hdr.data_len == 0x100
    assert hdr.flags == 0xAA55AA55
    assert hdr.features == 0x0003

    # Verify checksum
    expected_crc = crc32(blob[0:36])
    assert hdr.checksum == expected_crc, (
        f"checksum mismatch: header says 0x{hdr.checksum:08X}, "
        f"recomputed 0x{expected_crc:08X}"
    )

    out.unlink()


def test_python_crc_matches_c_known_answers():
    """C test_crc.c uses these exact inputs; Python must produce identical results."""
    assert crc32(b"123456789") == 0xCBF43926
    assert crc32(b"") == 0x00000000
    assert crc32(b"a") == 0xE8B7BE43
