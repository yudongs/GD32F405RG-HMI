"""Verify subcommand: run protocol compliance test (no hardware)."""

import json
import os
import sys
import subprocess
from pathlib import Path

import yaml


def run(config_path: str) -> bool:
    """Run the vtfp_proto verify suite (no hardware needed). Returns True on pass."""
    cfg = Path(config_path)
    if not cfg.exists():
        print(f"Error: {cfg} not found", file=sys.stderr)
        return False

    # Step 1: Validate config against schema
    schema_path = Path(__file__).resolve().parents[2] / "vtfp-protocol" / "vtfp.schema.json"
    if not schema_path.exists():
        print(f"Warning: schema not found at {schema_path}; skipping validation", file=sys.stderr)
    else:
        try:
            import jsonschema
            with open(cfg) as f:
                config = yaml.safe_load(f)
            with open(schema_path) as f:
                schema = json.load(f)
            jsonschema.Draft7Validator(schema).validate(config)
            print(f"OK: {cfg} validates against vtfp.schema.json")
        except ImportError:
            print("Warning: jsonschema not installed; skipping validation", file=sys.stderr)
        except jsonschema.ValidationError as e:
            print(f"FAIL: {cfg} does not validate: {e.message}")
            return False
        except Exception as e:
            print(f"Validation error: {e}", file=sys.stderr)
            return False

    # Step 2: Run vtfp_proto Python tests
    print("--- Running vtfp_proto tests ---")
    vtfp_proto_dir = Path(__file__).resolve().parents[2] / "vtfp-protocol" / "py"
    result = subprocess.run(
        [sys.executable, "-m", "pytest", "-q", "tests/"],
        cwd=str(vtfp_proto_dir), capture_output=True, text=True,
    )
    print(result.stdout)
    if result.returncode != 0:
        print(f"FAIL: vtfp_proto tests failed\n{result.stderr}", file=sys.stderr)
        return False
    print(f"OK: vtfp_proto tests passed")

    # Step 3: Run mklink_vtfp tests
    print("--- Running mklink_vtfp tests ---")
    mklink_dir = Path(__file__).resolve().parents[2] / "mklink-vtfp"
    result = subprocess.run(
        [sys.executable, "-m", "pytest", "-q", "tests/"],
        cwd=str(mklink_dir), capture_output=True, text=True,
    )
    print(result.stdout)
    if result.returncode != 0:
        print(f"FAIL: mklink_vtfp tests failed\n{result.stderr}", file=sys.stderr)
        return False
    print(f"OK: mklink_vtfp tests passed")

    return True
