"""Verify Python constants match the C header values."""

import pytest
from vtfp_proto import (
    VTFP_MAGIC, VTFP_VERSION, VTFP_DEFAULT_RAM_BASE,
    FEAT_LCD_CAPTURE, FEAT_RTT_LOG, FEAT_OTA_HOOK,
    CMD_USER_BASE, CMD_USER_MAX, CMD_STD_BASE, CMD_STD_MAX,
    CMD_SAFETY_BASE, CMD_SAFETY_MAX,
    CMD_QUERY_INFO, CMD_QUERY_STATE, CMD_RESET, CMD_HEARTBEAT,
    CMD_ARM, CMD_DISARM,
    R_OK, R_NOT_ARMED, R_ARM_EXPIRED, R_SAFETY_KEY,
    R_PARAM_RANGE, R_NOT_INIT, R_BUF_OVERFLOW, R_UNKNOWN_CMD,
    SAFETY_KEY, ARM_TIMEOUT_MS,
)


class TestMagic:
    def test_magic_value(self):
        # 'V','T','F','P' little-endian
        assert VTFP_MAGIC == 0x50465456

    def test_magic_individual_bytes(self):
        assert VTFP_MAGIC.to_bytes(4, "little") == b"VTFP"


class TestVersion:
    def test_version_value(self):
        assert VTFP_VERSION == 0x0001


class TestDefaultRamBase:
    def test_ram_base_value(self):
        assert VTFP_DEFAULT_RAM_BASE == 0x20000000


class TestFeatures:
    def test_lcd_capture_bit(self):
        assert FEAT_LCD_CAPTURE == 1

    def test_rtt_log_bit(self):
        assert FEAT_RTT_LOG == 2

    def test_ota_hook_bit(self):
        assert FEAT_OTA_HOOK == 4

    def test_features_are_distinct(self):
        bits = [FEAT_LCD_CAPTURE, FEAT_RTT_LOG, FEAT_OTA_HOOK]
        assert len(set(bits)) == 3


class TestCommandPartitions:
    def test_user_range(self):
        assert CMD_USER_BASE == 0x01
        assert CMD_USER_MAX == 0x0F

    def test_standard_range(self):
        assert CMD_STD_BASE == 0x10
        assert CMD_STD_MAX == 0x1F

    def test_safety_range(self):
        assert CMD_SAFETY_BASE == 0xF0
        assert CMD_SAFETY_MAX == 0xFF


class TestStandardCommands:
    @pytest.mark.parametrize("cmd_id,name", [
        (CMD_QUERY_INFO,  "QUERY_INFO"),
        (CMD_QUERY_STATE, "QUERY_STATE"),
        (CMD_RESET,       "RESET"),
        (CMD_HEARTBEAT,   "HEARTBEAT"),
    ])
    def test_standard_cmd_in_std_range(self, cmd_id, name):
        assert CMD_STD_BASE <= cmd_id <= CMD_STD_MAX

    def test_standard_cmds_distinct(self):
        cmds = {CMD_QUERY_INFO, CMD_QUERY_STATE, CMD_RESET, CMD_HEARTBEAT}
        assert len(cmds) == 4


class TestSafetyCommands:
    def test_arm_in_safety_range(self):
        assert CMD_SAFETY_BASE <= CMD_ARM <= CMD_SAFETY_MAX

    def test_disarm_in_safety_range(self):
        assert CMD_SAFETY_BASE <= CMD_DISARM <= CMD_SAFETY_MAX

    def test_arm_disarm_distinct(self):
        assert CMD_ARM != CMD_DISARM


class TestResultCodes:
    @pytest.mark.parametrize("code", [
        R_OK, R_NOT_ARMED, R_ARM_EXPIRED, R_SAFETY_KEY,
        R_PARAM_RANGE, R_NOT_INIT, R_BUF_OVERFLOW,
    ])
    def test_known_codes_in_low_range(self, code):
        assert 0 <= code <= 6

    def test_unknown_cmd_value(self):
        assert R_UNKNOWN_CMD == 0xFF


class TestSafetyPrimitives:
    def test_safety_key_value(self):
        assert SAFETY_KEY == 0xAA55AA55

    def test_arm_timeout_value(self):
        assert ARM_TIMEOUT_MS == 2000
