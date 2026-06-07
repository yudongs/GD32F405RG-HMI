"""Ensure src/ is on sys.path so tests can import the package without install."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]  # .../vtfp-protocol/py
sys.path.insert(0, str(ROOT / "src"))
