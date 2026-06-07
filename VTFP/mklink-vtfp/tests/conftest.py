"""Ensure mklink_vtfp package is importable for tests."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # .../mklink-vtfp
sys.path.insert(0, str(ROOT / "src"))
