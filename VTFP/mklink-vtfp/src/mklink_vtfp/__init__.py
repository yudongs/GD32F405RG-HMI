"""mklink-vtfp — Python SDK for VTFP v1 protocol.

This package provides a high-level Python API for embedded MCUs running
the VTFP v1 firmware runtime. It wraps:
- vtfp_proto (M1): low-level protocol primitives (constants, header, CRC)
- MKLink transport: serial bridge + dump memory protocol
"""

from .transport import MKLinkSerialBridge, PortBusyError
from .transport import get_default_port, get_default_baudrate
from .sdk import (
    VTFPAgent,
    VTFPAgentError,
    VTFPAgentNotFoundError,
    VTFPAgentVersionError,
    VTFPAgentCorruptError,
    VTFPAgentTimeoutError,
    VTFPAgentSafetyError,
    VTFPAgentDataError,
    VTFPAgentUnknownCmdError,
    QueryInfoResult,
    CommandResult,
    load_project_config,
)

__version__ = "0.1.0"

__all__ = [
    # Transport
    "MKLinkSerialBridge", "PortBusyError", "get_default_port", "get_default_baudrate",
    # Agent
    "VTFPAgent", "load_project_config",
    # Exceptions
    "VTFPAgentError", "VTFPAgentNotFoundError", "VTFPAgentVersionError",
    "VTFPAgentCorruptError", "VTFPAgentTimeoutError", "VTFPAgentSafetyError",
    "VTFPAgentDataError", "VTFPAgentUnknownCmdError",
    # Result types
    "QueryInfoResult", "CommandResult",
]
