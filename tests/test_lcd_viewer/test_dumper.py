"""Tests for tools.lcd_viewer.dumper — subprocess lifecycle + frame dispatch.

We patch `subprocess.Popen` to return a mock whose `.stdout` is an iterable
of strings (JSON lines), simulating dump-memory's output. We then drive
the Dumper through start → frames → stop and assert state changes.

Protocol: each line is one B1 chunk of a sample. For a 4800-byte region with
2048-byte blocks, a full sample = 3 lines (block_index 0, 1, 2). The reader
must accumulate blocks and only publish a complete Frame.
"""
from __future__ import annotations

import asyncio
import json
from unittest.mock import MagicMock, patch

import pytest

from tools.lcd_viewer import config
from tools.lcd_viewer.dumper import Dumper, DumperState
from tools.lcd_viewer.frame import Frame


def _make_block_line(
    block_index: int,
    block_count: int = 3,
    ts_us: int = 100,
    size: int = 2048,
    fill: int = 0x00,
) -> str:
    """Build one mklink-format dump-memory JSON line (one B1 chunk)."""
    obj = {
        "timestamp_us": ts_us,
        "format": "B1",
        "flags": 0,
        "regions": [{
            "index": 0,
            "address": "0x20001BA4",
            "size": size,
            "hex": " ".join(f"{fill:02X}" for _ in range(size)),
        }],
        "total_size": 4800,
        "block_size": 2048,
        "block_index": block_index,
        "block_count": block_count,
        "block_crc_ok": True,
    }
    return json.dumps(obj)


def _make_sample_lines(ts_us: int, fill: int = 0x00) -> list[str]:
    """Build the 3 B1 lines for a single 4800-byte sample."""
    return [
        _make_block_line(0, 3, ts_us, 2048, fill),
        _make_block_line(1, 3, ts_us, 2048, fill),
        _make_block_line(2, 3, ts_us, 704, fill),
    ]


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
        # 3 lines = one full sample
        mock_popen.return_value = _make_mock_proc(_make_sample_lines(ts_us=123))
        await d.start(period_ms=50)
        # Allow the reader task to process all 3 blocks.
        await asyncio.sleep(0.05)

    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 123
    assert d.state == DumperState.RUNNING
    assert any(s == DumperState.STARTING for s, _ in state_changes)
    assert any(s == DumperState.RUNNING for s, _ in state_changes)

    await d.stop()


@pytest.mark.asyncio
async def test_dumper_does_not_publish_partial_sample():
    """Feeding only 1 or 2 blocks must NOT publish a Frame."""
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        # Only 2 of 3 blocks for a sample
        mock_popen.return_value = _make_mock_proc([
            _make_block_line(0, 3, ts_us=99),
            _make_block_line(1, 3, ts_us=99),
        ])
        await d.start(period_ms=50)
        await asyncio.sleep(0.05)
    assert d.latest_frame is None
    assert d.state == DumperState.STARTING  # still waiting
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
    """A malformed JSON line must not crash the reader; the next good block wins."""
    d = Dumper()
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([
            "not json",  # ignored
            *_make_sample_lines(ts_us=99, fill=0xCC),  # accepted
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


@pytest.mark.asyncio
async def test_dumper_invokes_new_frame_callback_per_published_frame():
    """on_new_frame() must be called once per *published* (complete) Frame.

    This is the contract WSServer relies on for event-driven broadcast.
    """
    callback_count = 0

    def on_new_frame() -> None:
        nonlocal callback_count
        callback_count += 1

    d = Dumper()
    d.set_new_frame_callback(on_new_frame)
    # 3 samples × 3 blocks each = 9 lines, but only 3 should publish a Frame.
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([
            *_make_sample_lines(ts_us=1, fill=0x11),
            *_make_sample_lines(ts_us=2, fill=0x22),
            *_make_sample_lines(ts_us=3, fill=0x33),
        ])
        await d.start(period_ms=50)
        # Allow the reader task to consume all 9 lines.
        await asyncio.sleep(0.1)

    assert callback_count == 3, (
        f"expected 3 callback invocations (one per Frame), got {callback_count}"
    )
    await d.stop()


@pytest.mark.asyncio
async def test_dumper_new_frame_callback_errors_do_not_break_reader():
    """A misbehaving callback must not stop the reader from processing frames."""
    def on_new_frame() -> None:
        raise RuntimeError("boom")

    d = Dumper()
    d.set_new_frame_callback(on_new_frame)
    with patch("tools.lcd_viewer.dumper.subprocess.Popen") as mock_popen:
        mock_popen.return_value = _make_mock_proc([
            *_make_sample_lines(ts_us=10, fill=0xAA),
            *_make_sample_lines(ts_us=11, fill=0xBB),
        ])
        await d.start(period_ms=50)
        await asyncio.sleep(0.1)

    # Reader kept going — last published frame is the second one.
    assert d.latest_frame is not None
    assert d.latest_frame.ts_us == 11
    await d.stop()
