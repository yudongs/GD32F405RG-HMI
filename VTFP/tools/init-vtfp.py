#!/usr/bin/env python3
"""init-vtfp.py - entry point for the VTFP v1范本 scaffolding tool.

Usage:
    python init-vtfp.py discover   [--root <path>]
    python init-vtfp.py interview  [--root <path>] [--resume]
    python init-vtfp.py codegen    [--config vtfp.yaml] [--diff] [--out-dir <path>]
    python init-vtfp.py verify     [--config vtfp.yaml]

Subcommands:
    discover   Scan a project to detect RTOS, existing MKLink, etc.
               Does NOT modify any files.
    interview  Interactive Q&A to generate vtfp.yaml.
               Writes vtfp.yaml + .vtfp-state.json.
    codegen    Read vtfp.yaml, write C dispatch + Python SDK + MCP server.
               --diff  dry-run, show what would change.
    verify     Run protocol verification (mock MCU side).
"""

import sys
import argparse
import os
from pathlib import Path

# Make init_vtfp importable
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from init_vtfp import discover, interview, codegen, verify


def main():
    parser = argparse.ArgumentParser(
        prog="init-vtfp.py",
        description="VTFP v1 scaffolding tool",
    )
    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    # discover
    p = subparsers.add_parser("discover", help="Scan project to detect RTOS, etc.")
    p.add_argument("--root", default=".", help="Project root path (default: cwd)")

    # interview
    p = subparsers.add_parser("interview", help="Interactive Q&A to generate vtfp.yaml")
    p.add_argument("--root", default=".", help="Project root path")
    p.add_argument("--resume", action="store_true", help="Resume from saved state")

    # codegen
    p = subparsers.add_parser("codegen", help="Generate C + Python + MCP from vtfp.yaml")
    p.add_argument("--config", default="vtfp.yaml", help="Path to vtfp.yaml")
    p.add_argument("--out-dir", default=".", help="Output directory")
    p.add_argument("--diff", action="store_true", help="Dry-run, show what would change")

    # verify
    p = subparsers.add_parser("verify", help="Run protocol compliance test (no hardware)")
    p.add_argument("--config", default="vtfp.yaml", help="Path to vtfp.yaml")

    args = parser.parse_args()

    if args.subcommand == "discover":
        result = discover.run(args.root)
        print(result.to_yaml())
    elif args.subcommand == "interview":
        config = interview.run(args.root, resume=args.resume)
        out_path = Path(args.root) / "vtfp.yaml"
        out_path.write_text(config.to_yaml(), encoding="utf-8")
        print(f"Wrote {out_path}")
    elif args.subcommand == "codegen":
        codegen.run(args.config, args.out_dir, diff=args.diff)
    elif args.subcommand == "verify":
        ok = verify.run(args.config)
        sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
