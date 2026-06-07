"""Persistent state for the interview subcommand (resume support)."""

import json
from pathlib import Path
from typing import Any, Dict, Optional


def state_path(root: str, project_id: str = "default") -> Path:
    return Path(root) / ".vtfp-state" / project_id / "interview.json"


def load(root: str, project_id: str = "default") -> Optional[Dict[str, Any]]:
    p = state_path(root, project_id)
    if not p.exists():
        return None
    try:
        return json.loads(p.read_text(encoding="utf-8"))
    except Exception:
        return None


def save(root: str, answers: Dict[str, Any], project_id: str = "default") -> None:
    p = state_path(root, project_id)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(answers, indent=2, ensure_ascii=False), encoding="utf-8")


def clear(root: str, project_id: str = "default") -> None:
    p = state_path(root, project_id)
    if p.exists():
        p.unlink()
