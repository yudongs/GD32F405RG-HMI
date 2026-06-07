"""Tests for the transport layer (port_config, bridge)."""

import os
import sys
import pytest
from unittest.mock import MagicMock, patch

from mklink_vtfp.transport import get_default_port, get_default_baudrate
from mklink_vtfp.transport.bridge import MKLinkSerialBridge, PortBusyError


class TestPortConfig:
    def test_default_port_no_env(self, monkeypatch):
        monkeypatch.delenv("MKLINK_PORT", raising=False)
        assert get_default_port() == "COM5"

    def test_default_port_with_env(self, monkeypatch):
        monkeypatch.setenv("MKLINK_PORT", "COM7")
        assert get_default_port() == "COM7"

    def test_default_baud_no_env(self, monkeypatch):
        monkeypatch.delenv("MKLINK_BAUD", raising=False)
        assert get_default_baudrate() == 115200

    def test_default_baud_with_env(self, monkeypatch):
        monkeypatch.setenv("MKLINK_BAUD", "9600")
        assert get_default_baudrate() == 9600

    def test_custom_default_port(self, monkeypatch):
        monkeypatch.delenv("MKLINK_PORT", raising=False)
        assert get_default_port(default="/dev/ttyUSB0") == "/dev/ttyUSB0"


class TestBridgeMocked:
    """Test MKLinkSerialBridge with a mocked pyserial.Serial."""

    def test_bridge_constructs_without_connecting(self):
        b = MKLinkSerialBridge("COM5", baudrate=115200)
        assert b.port == "COM5"
        assert b.baudrate == 115200
        assert b._ser is None

    def test_send_command_with_mock(self):
        b = MKLinkSerialBridge("COM5")
        mock_ser = MagicMock()
        # Simulate one response line
        mock_ser.in_waiting = 100
        mock_ser.read.return_value = b"DEADBEEF\n"
        b._ser = mock_ser
        result = b.send_command("cmd.read_ram(0x20000000, 4)")
        assert "DEADBEEF" in result
        mock_ser.write.assert_called_once()

    def test_port_busy_translates_to_friendly_error(self):
        b = MKLinkSerialBridge("COM5")
        with patch("serial.Serial", side_effect=Exception("Access denied")):
            with pytest.raises(PortBusyError, match="Cannot open"):
                b.connect()
