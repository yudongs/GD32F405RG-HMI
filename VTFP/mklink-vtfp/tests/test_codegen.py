"""Tests for the codegen module."""

import subprocess
import sys
from pathlib import Path

import pytest

from mklink_vtfp.codegen import generate_mcp_server, generate_c_dispatch

HERE = Path(__file__).resolve().parent
PKG = HERE.parent

SAMPLE_CONFIG = {
    "project": {"name": "test", "product_id": "TEST-1.0", "model_code": 0},
    "protocol": {"version": "0.1.0", "features": []},
    "memory": {"header_base": "0x20000000", "data_buffer_size": 1024, "alignment": 4},
    "safety": {"arm_timeout_ms": 2000, "safety_key": "0xAA55AA55", "auto_disarm": True},
    "rtos": {"type": "rt-thread", "tick_ms": 20},
    "commands": [
        {
            "id": 1,
            "name": "TOGGLE_OUTPUT",
            "description": "Toggle a digital output",
            "direction": "pc_to_mcu",
            "requires_arm": True,
            "request_schema": {"type": "int", "size_bytes": 4},
        },
        {
            "id": 2,
            "name": "READ_SENSOR",
            "description": "Read a sensor value",
            "direction": "pc_to_mcu",
            "requires_arm": False,
        },
    ],
}


def test_generate_mcp_server_writes_file():
    out = HERE / "_test_mcp.py"
    try:
        result = generate_mcp_server(SAMPLE_CONFIG, str(out))
        assert Path(result).exists()
        content = out.read_text(encoding="utf-8")
        # Sanity checks on generated content
        assert "FastMCP" in content
        assert "mcp.tool" in content
        assert "TOGGLE_OUTPUT" in content or "toggle_output" in content
        assert "READ_SENSOR" in content or "read_sensor" in content
        assert "arm" in content
        assert "disarm" in content
        assert "query_info" in content
    finally:
        if out.exists():
            out.unlink()


def test_generate_mcp_server_uses_custom_product_id():
    out = HERE / "_test_mcp2.py"
    try:
        cfg = dict(SAMPLE_CONFIG)
        cfg = {**SAMPLE_CONFIG, "project": {**SAMPLE_CONFIG["project"], "product_id": "CUSTOM-9.9"}}
        generate_mcp_server(cfg, str(out))
        content = out.read_text(encoding="utf-8")
        assert "CUSTOM-9.9" in content
    finally:
        if out.exists():
            out.unlink()


def test_generate_c_dispatch_delegates():
    """Verify generate_c_dispatch produces a C file with the right command names."""
    out = HERE / "_test_dispatch.c"
    try:
        result = generate_c_dispatch(SAMPLE_CONFIG, str(out))
        assert Path(result).exists()
        # gen_c_dispatch.py writes with platform default encoding (GBK on Chinese Windows).
        # Use errors="replace" so the test is robust to non-UTF-8 bytes (em dash, etc.).
        content = out.read_text(encoding="utf-8", errors="replace")
        # The C codegen produces functions like my_toggle_output_handler
        assert "my_toggle_output_handler" in content
        assert "my_read_sensor_handler" in content
    finally:
        if out.exists():
            out.unlink()


def test_cli_entrypoint_runs():
    """Verify the CLI 'python -m mklink_vtfp.codegen' works end-to-end."""
    out_mcp = HERE / "_test_cli_mcp.py"
    out_c   = HERE / "_test_cli_dispatch.c"
    yaml_file = HERE / "_test_cli.yaml"
    try:
        import yaml
        yaml_file.write_text(yaml.safe_dump(SAMPLE_CONFIG), encoding="utf-8")
        result = subprocess.run(
            [sys.executable, "-m", "mklink_vtfp.codegen", str(yaml_file), str(out_mcp), str(out_c)],
            capture_output=True, text=True, cwd=str(PKG),
        )
        assert result.returncode == 0, f"CLI failed: {result.stderr}"
        assert out_mcp.exists()
        assert out_c.exists()
    finally:
        for f in (out_mcp, out_c, yaml_file):
            if f.exists():
                f.unlink()
