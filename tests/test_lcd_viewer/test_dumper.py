"""Tests for tools.lcd_viewer.dumper — subprocess lifecycle + frame dispatch.

We patch `subprocess.Popen` to return a mock whose `.stdout` is an iterable
of strings (JSON lines), simulating dump-memory's output. We then drive
the Dumper through start → frames → stop and assert state changes.
"""
from __future__ import annotations

import asyncio
import base64
import json
import sys
from unittest.mock import MagicMock, patch

import pytest

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper, DumperState
from tools.lcd_viewer.frame import Frame


def _make_json_line(ts_us: int = 100) -> str:
    blocks = [
        {"offset": 0, "size": 2048, "payload_b64": base64.b64encode(b"\x00" * 2048).decode()},
        {"offset": 2048, "size": 2048, "payload_b64": base64.b64encode(b"\x00" * 2048).decode()},
        {"offset": 4096, "size": 704, "payload_b64": base64.b64encode(b"\x00" * 704).decode()},
    ]
    return json.dumps({"ts_us": ts_us, "blocks": blocks})


def _make_mock_proc(stdout_lines: list[str], returncode: int = 0) -> MagicMock:
    """Build a mock subprocess.Popen that yields the given stdout lines then exits."""
    proc = MagicMock()
    proc.stdout = iter(stdout_lines)  # blocking iterator
    proc.poll.return_value = None
    proc.returncode = returncode
    proc.wait.return_value = returncode
    return proc


@pytest.mark.asyncio
async def test_dumper_starts_in_idle_state():
    d = Dumper()
    assert d.state == DumperState.IDLE
    assert d.latest_frame is None


@pytest.mark.asyncio
async def test_dumper_transitions_to_running_on_first_frame():
    state_changes: list[tuple[DumperState, str]] = []

    def on_state(state, msg):
        state_changes.append((state, msg))

    d = Dumper(on_state_change=on_state)
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([_make_json_line(123)])
        await d.start(period_ms=50)
        # Allow the reader task to process one frame.
        await asyncio.sleep(0.05)

    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 123
    assert d.state == DumperState.RUNNING
    assert any(s == DumperState.STARTING for s, _ in state_changes)
    assert any(s == DumperState.RUNNING for s, _ in state_changes)

    await d.stop()


@pytest.mark.asyncio
async def test_dumper_stop_terminates_subprocess():
    """Graceful exit path: proc.wait() returns 0 → no kill needed."""
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_proc = _make_mock_proc([])  # empty stdout
        mock_popen.return_value = mock_proc
        await d.start(period_ms=50)
        await d.stop()
    mock_proc.kill.assert_not_called()
    assert d.state == DumperState.IDLE


@pytest.mark.asyncio
async def test_dumper_force_kills_on_timeout():
    """If the subprocess doesn't exit within 2s, Popen.kill() must be called."""
    import subprocess

    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_proc = _make_mock_proc([])
        mock_proc.wait.side_effect = subprocess.TimeoutExpired(cmd="x", timeout=2.0)
        mock_popen.return_value = mock_proc
        await d.start(period_ms=50)
        await d.stop()
    mock_proc.kill.assert_called_once()
    # After kill, wait is called again to reap the zombie.
    assert mock_proc.wait.call_count >= 2


@pytest.mark.asyncio
async def test_dumper_handles_parse_errors_gracefully():
    """A malformed JSON line must not crash the reader; the next good line wins."""
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([
            "not json",                       # ignored
            _make_json_line(ts_us=99),        # accepted
        ])
        await d.start(period_ms=50)
        await asyncio.sleep(0.05)
    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 99
    await d.stop()


@pytest.mark.asyncio
async def test_dumper_restart_replaces_subprocess():
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([])
        await d.start(period_ms=50)
        first_proc = mock_popen.return_value
        # Calling start again with different period triggers stop+start.
        mock_popen.return_value = _make_mock_proc([])
        await d.start(period_ms=100)
        second_proc = mock_popen.return_value
    assert first_proc is not second_proc
    # Two different Popen() calls were made.
    assert mock_popen.call_count == 2
    await d.stop()


def test_dumper_state_enum_has_required_values():
    assert DumperState.IDLE
    assert DumperState.STARTING
    assert DumperState.RUNNING
    assert DumperState.STOPPING
    assert DumperState.ERROR
