"""Tests for init-vtfp.py."""

import os
import sys
import json
import subprocess
from pathlib import Path

import pytest


HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent
PROJECT_ROOT = TOOLS.parent
INIT_VTFP = TOOLS / "init-vtfp.py"


class TestDiscover:
    def test_discover_runs(self, tmp_path, capsys):
        """Run `init-vtfp.py discover` against a fake project root and check output."""
        # Create a fake project: just an empty directory
        result = subprocess.run(
            [sys.executable, str(INIT_VTFP), "discover", "--root", str(tmp_path)],
            capture_output=True, text=True, cwd=str(tmp_path),
        )
        assert result.returncode == 0, f"discover failed: {result.stderr}"
        # Output should be valid YAML containing the expected fields
        import yaml
        out = yaml.safe_load(result.stdout)
        assert "project_root" in out
        assert "rtos" in out
        assert "toolchain" in out

    def test_discover_detects_rtthread(self, tmp_path):
        """A directory with rtthread.h should be detected as rt-thread."""
        (tmp_path / "rtthread.h").write_text("// fake RT-Thread header\n")
        result = subprocess.run(
            [sys.executable, str(INIT_VTFP), "discover", "--root", str(tmp_path)],
            capture_output=True, text=True, cwd=str(tmp_path),
        )
        import yaml
        out = yaml.safe_load(result.stdout)
        assert out["rtos"] == "rt-thread"


class TestInterview:
    def test_interview_writes_vtfp_yaml(self, tmp_path, monkeypatch):
        """Run interview with simulated input, verify vtfp.yaml is written."""
        inputs = [
            "test-controller",     # project name
            "TEST-1.0",            # product_id
            "",                    # model_code (use default)
            "",                    # rtos (use default rt-thread)
            "",                    # tick_ms (use default)
            "",                    # arm_timeout_ms
            "",                    # safety_key
            "",                    # auto_disarm
            "",                    # header_base
            "",                    # data_buffer_size
            "",                    # alignment
            "",                    # first command name (empty = stop)
        ]
        result = subprocess.run(
            [sys.executable, str(INIT_VTFP), "interview", "--root", str(tmp_path)],
            input="\n".join(inputs) + "\n",
            capture_output=True, text=True, cwd=str(tmp_path),
        )
        assert result.returncode == 0, f"interview failed: {result.stderr}\n{result.stdout}"
        yaml_path = tmp_path / "vtfp.yaml"
        assert yaml_path.exists()
        import yaml
        cfg = yaml.safe_load(yaml_path.read_text(encoding="utf-8"))
        assert cfg["project"]["name"] == "test-controller"
        assert cfg["project"]["product_id"] == "TEST-1.0"


class TestVerify:
    def test_verify_passes(self, tmp_path):
        """A minimal valid vtfp.yaml should make verify pass."""
        # Write a minimal vtfp.yaml
        yaml_path = tmp_path / "vtfp.yaml"
        yaml_path.write_text("""\
project:
  name: verify-test
  product_id: VERIFY-1.0
protocol:
  version: "0.1.0"
memory:
  header_base: "0x20000000"
  data_buffer_size: 1024
commands:
  - id: 1
    name: QUERY_INFO
    direction: pc_to_mcu
    requires_arm: false
""", encoding="utf-8")

        result = subprocess.run(
            [sys.executable, str(INIT_VTFP), "verify", "--config", str(yaml_path)],
            capture_output=True, text=True,
        )
        # Verify should pass (it just runs vtfp_proto + mklink_vtfp pytest)
        assert result.returncode == 0, f"verify failed: {result.stderr}\n{result.stdout}"

    def test_verify_rejects_bad_yaml(self, tmp_path):
        """A yaml that doesn't match the schema should fail validation."""
        yaml_path = tmp_path / "vtfp.yaml"
        yaml_path.write_text("project: {name: missing-required-fields}", encoding="utf-8")

        result = subprocess.run(
            [sys.executable, str(INIT_VTFP), "verify", "--config", str(yaml_path)],
            capture_output=True, text=True,
        )
        # Should fail (missing required protocol, memory, commands)
        assert result.returncode != 0
        assert "FAIL" in result.stdout or "validation" in result.stdout.lower()
