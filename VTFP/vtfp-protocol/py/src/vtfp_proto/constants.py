"""VTFP v1 constants. Must match vtfp-protocol/c-types/ byte-for-byte."""

# Magic: 'V','T','F','P' as little-endian uint32
VTFP_MAGIC = 0x50465456

# Protocol version
VTFP_VERSION = 0x0001

# Default header base (STM32F4 SRAM start)
VTFP_DEFAULT_RAM_BASE = 0x20000000

# Feature bitmap
FEAT_LCD_CAPTURE = 1 << 0
FEAT_RTT_LOG     = 1 << 1
FEAT_OTA_HOOK    = 1 << 2

# Command ID partition
CMD_USER_BASE,   CMD_USER_MAX   = 0x01, 0x0F
CMD_STD_BASE,    CMD_STD_MAX    = 0x10, 0x1F
CMD_SAFETY_BASE, CMD_SAFETY_MAX = 0xF0, 0xFF

# Standard commands
CMD_QUERY_INFO  = 0x10
CMD_QUERY_STATE = 0x11
CMD_RESET       = 0x12
CMD_HEARTBEAT   = 0x13

# Safety commands
CMD_ARM    = 0xFE
CMD_DISARM = 0xFF

# Result codes
R_OK            = 0
R_NOT_ARMED     = 1
R_ARM_EXPIRED   = 2
R_SAFETY_KEY    = 3
R_PARAM_RANGE   = 4
R_NOT_INIT      = 5
R_BUF_OVERFLOW  = 6
R_UNKNOWN_CMD   = 0xFF

# Safety
SAFETY_KEY      = 0xAA55AA55
ARM_TIMEOUT_MS  = 2000
