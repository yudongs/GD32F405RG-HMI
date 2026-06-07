"""Codegen subcommand: invoke mklink_vtfp codegen to write C + Python + MCP server."""

import os
import sys
import subprocess
from pathlib import Path
from typing import Optional

import yaml

# Make mklink_vtfp importable
HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent
PROJECT_ROOT = TOOLS.parent
MKTF = PROJECT_ROOT / "mklink-vtfp" / "src"
VTFP_PROTO = PROJECT_ROOT / "vtfp-protocol" / "py" / "src"
VTFPRT = PROJECT_ROOT / "vtfp-runtime"

for p in (str(MKTF), str(VTFP_PROTO)):
    if p not in sys.path:
        sys.path.insert(0, p)


def _diff_lines(new_content: str, existing: Optional[str]) -> str:
    """Return a unified diff string for human reading."""
    import difflib
    if existing is None:
        return f"+++ (new file)\n{new_content[:500]}{'...' if len(new_content) > 500 else ''}"
    diff = difflib.unified_diff(
        existing.splitlines(keepends=True),
        new_content.splitlines(keepends=True),
        fromfile="existing",
        tofile="new",
    )
    return "".join(diff)


def run(config_path: str, out_dir: str, diff: bool = False) -> None:
    """Read vtfp.yaml and write C dispatch + Python SDK + MCP server."""
    cfg_path = Path(config_path)
    if not cfg_path.exists():
        print(f"Error: {cfg_path} not found. Run `init-vtfp.py interview` first.", file=sys.stderr)
        sys.exit(1)

    with open(cfg_path, encoding="utf-8") as f:
        config = yaml.safe_load(f)

    out_root = Path(out_dir).resolve()
    out_root.mkdir(parents=True, exist_ok=True)

    # Generate C dispatch
    c_out = out_root / "vtfp_user_dispatch.c"
    new_c = gen_c_dispatch_file(config, c_out.name)
    if diff:
        existing = c_out.read_text(encoding="utf-8") if c_out.exists() else None
        print(f"--- {c_out} ---")
        print(_diff_lines(new_c, existing))
    else:
        c_out.write_text(new_c, encoding="utf-8")
        print(f"Generated: {c_out}")

    # Generate Python SDK (no-op; mklink_vtfp is the SDK)
    # Generate MCP server
    try:
        from mklink_vtfp.codegen import generate_mcp_server
        mcp_out = out_root / "mcp_server.py"
        if diff:
            # We can't easily preview without writing; skip MCP diff
            print(f"--- {mcp_out} (would be generated) ---")
        else:
            generate_mcp_server(config, str(mcp_out))
            print(f"Generated: {mcp_out}")
    except ImportError:
        print("Warning: mklink_vtfp not installed; skipping MCP server generation", file=sys.stderr)


# The C-side codegen lives in vtfp-runtime/codegen/gen_c_dispatch.py
# We re-implement the minimal glue here to avoid an import dance.
def gen_c_dispatch_file(config: dict, filename: str) -> str:
    """Generate a C dispatch source from a vtfp.yaml config.

    Returns the C source as a string. Caller writes to disk.
    """
    import sys as _sys
    _sys.path.insert(0, str(VTFPRT / "codegen"))
    import importlib.util
    spec = importlib.util.spec_from_file_location("gen_c_dispatch", str(VTFPRT / "codegen" / "gen_c_dispatch.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    # Use a temp file to capture output
    import tempfile, os as _os
    with tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False, encoding="utf-8") as f:
        yaml.safe_dump(config, f)
        tmp_yaml = f.name
    out_tmp = tempfile.NamedTemporaryFile("w", suffix=".c", delete=False, encoding="utf-8")
    out_tmp.close()
    try:
        # Call the gen function directly
        import io
        from contextlib import redirect_stdout
        buf = io.StringIO()
        # The C-side gen_c_dispatch.py is a script; refactor: read its main logic
        # Simpler: directly call its main with sys.argv
        old_argv = _sys.argv
        _sys.argv = ["gen_c_dispatch.py", tmp_yaml, out_tmp.name]
        try:
            with redirect_stdout(buf):
                mod.main()
        finally:
            _sys.argv = old_argv
        c_source = open(out_tmp.name, encoding="utf-8").read()
        return c_source
    finally:
        _os.unlink(tmp_yaml)
        try:
            _os.unlink(out_tmp.name)
        except FileNotFoundError:
            pass
