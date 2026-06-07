"""Port configuration from environment variables and defaults."""

import os
from typing import Optional


def get_default_port(env_var: str = "MKLINK_PORT", default: str = "COM5") -> str:
    """Get the default MKLink serial port.

    Resolution order:
    1. `env_var` env var (default: MKLINK_PORT)
    2. `default` parameter (default: "COM5")
    """
    return os.environ.get(env_var, default)


def get_default_baudrate(default: int = 115200) -> int:
    """Get the default baud rate."""
    return int(os.environ.get("MKLINK_BAUD", str(default)))
