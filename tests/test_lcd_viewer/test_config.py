"""Tests for tools.lcd_viewer.config — verifies the runtime constants."""
from tools.lcd_viewer import config


def test_fb_address_is_in_ram():
    # 0x20000000-0x20020000 is the GD32F405 SRAM region.
    assert 0x20000000 <= config.FB_ADDR < 0x20020000


def test_fb_size_matches_geometry():
    assert config.FB_SIZE == config.FB_WIDTH * config.FB_HEIGHT // 8


def test_default_period_within_bounds():
    assert config.MIN_PERIOD_MS <= config.DEFAULT_PERIOD_MS <= config.MAX_PERIOD_MS


def test_server_binds_to_localhost():
    assert config.HOST == "127.0.0.1"
    assert 1 <= config.PORT <= 65535


def test_ws_path_starts_with_slash():
    assert config.WS_PATH.startswith("/")
