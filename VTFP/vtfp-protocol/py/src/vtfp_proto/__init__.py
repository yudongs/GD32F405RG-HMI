"""VTFP v1 protocol primitives — public API.

This package is the Python counterpart of `vtfp-protocol/c-types/`. It
defines:
- Magic, version, and feature bit constants
- Command ID and partition boundaries
- Result codes
- Header dataclass with pack/unpack (matches `vtfp_header_t` byte-for-byte)
- CRC-32 (matches the C implementation)

M2 and M3 layers depend on this package; this package depends only on
the Python standard library.
"""

from .header import VTFHeader, pack_header, unpack_header, HEADER_SIZE
from .crc import crc32

from .constants import (
    VTFP_MAGIC,
    VTFP_VERSION,
    VTFP_DEFAULT_RAM_BASE,
    FEAT_LCD_CAPTURE,
    FEAT_RTT_LOG,
    FEAT_OTA_HOOK,
    CMD_USER_BASE, CMD_USER_MAX,
    CMD_STD_BASE, CMD_STD_MAX,
    CMD_SAFETY_BASE, CMD_SAFETY_MAX,
    CMD_QUERY_INFO, CMD_QUERY_STATE, CMD_RESET, CMD_HEARTBEAT,
    CMD_ARM, CMD_DISARM,
    R_OK, R_NOT_ARMED, R_ARM_EXPIRED, R_SAFETY_KEY,
    R_PARAM_RANGE, R_NOT_INIT, R_BUF_OVERFLOW, R_UNKNOWN_CMD,
    SAFETY_KEY, ARM_TIMEOUT_MS,
)

__version__ = "0.1.0"

__all__ = [
    "VTFP_MAGIC", "VTFP_VERSION", "VTFP_DEFAULT_RAM_BASE",
    "FEAT_LCD_CAPTURE", "FEAT_RTT_LOG", "FEAT_OTA_HOOK",
    "CMD_USER_BASE", "CMD_USER_MAX", "CMD_STD_BASE", "CMD_STD_MAX",
    "CMD_SAFETY_BASE", "CMD_SAFETY_MAX",
    "CMD_QUERY_INFO", "CMD_QUERY_STATE", "CMD_RESET", "CMD_HEARTBEAT",
    "CMD_ARM", "CMD_DISARM",
    "R_OK", "R_NOT_ARMED", "R_ARM_EXPIRED", "R_SAFETY_KEY",
    "R_PARAM_RANGE", "R_NOT_INIT", "R_BUF_OVERFLOW", "R_UNKNOWN_CMD",
    "SAFETY_KEY", "ARM_TIMEOUT_MS",
    "VTFHeader", "pack_header", "unpack_header", "HEADER_SIZE",
    "crc32",
]
