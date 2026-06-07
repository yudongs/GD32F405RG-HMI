"""Discover subcommand: scan a project to detect RTOS, existing MKLink, etc."""

import os
import re
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import List, Optional, Dict, Any


@dataclass
class DiscoverResult:
    project_root: str
    rtos: str = "unknown"               # rt-thread | freertos | bare | unknown
    toolchain: str = "unknown"          # keil | iar | gcc | unknown
    has_mklink_resource: bool = False
    has_test_fixture: bool = False
    rtos_version: str = ""
    main_file: str = ""
    warnings: List[str] = field(default_factory=list)
    recommendations: Dict[str, str] = field(default_factory=dict)

    def to_yaml(self) -> str:
        """Render as YAML for human inspection."""
        import yaml
        return yaml.safe_dump(asdict(self), allow_unicode=True, sort_keys=False, default_flow_style=False)


def detect_rtos(root: Path) -> tuple[str, str]:
    """Detect RTOS by scanning for headers. Returns (rtos, version_string)."""
    # RT-Thread
    for header in root.rglob("rtthread.h"):
        return ("rt-thread", "")
    # FreeRTOS
    for header in root.rglob("FreeRTOS.h"):
        return ("freertos", "")
    # Bare metal: has main() but no RTOS header
    has_main = any(root.rglob("main.c")) or any(root.rglob("main.cpp"))
    if has_main:
        return ("bare", "")
    return ("unknown", "")


def detect_toolchain(root: Path) -> str:
    """Detect build toolchain."""
    if any(root.rglob("*.uvprojx")):
        return "keil"
    if any(root.rglob("*.ewp")):
        return "iar"
    if any(root.rglob("SConstruct")) or any(root.rglob("SConscript")):
        return "gcc"
    return "unknown"


def detect_mklink(root: Path) -> tuple[bool, bool]:
    """Detect existing MKLink resource and test fixture."""
    has_resource = any(root.rglob("mklink_resource.c")) or any(root.rglob("mklink_resource.h"))
    has_fixture = any(root.rglob("test_fixture.c")) or any(root.rglob("test_fixture.h"))
    return (has_resource, has_fixture)


def detect_main_file(root: Path) -> str:
    """Find the project's main entry point."""
    for candidate in ("main.c", "Main.c", "src/main.c", "application/main.c"):
        p = root / candidate
        if p.exists():
            return str(p.relative_to(root))
    return ""


def run(root: str) -> DiscoverResult:
    """Scan the project and return a DiscoverResult."""
    root_path = Path(root).resolve()
    rtos, rtos_ver = detect_rtos(root_path)
    toolchain = detect_toolchain(root_path)
    has_resource, has_fixture = detect_mklink(root_path)
    main_file = detect_main_file(root_path)

    warnings: List[str] = []
    recs: Dict[str, str] = {}

    if rtos == "unknown":
        warnings.append("Could not detect RTOS - no rtthread.h, FreeRTOS.h, or main.c found")
        recs["rtos"] = "Manual: edit vtfp.yaml rtos.type after interview"
    if toolchain == "unknown":
        warnings.append("Could not detect toolchain - no .uvprojx, .ewp, or SConstruct found")
        recs["toolchain"] = "Manual: configure build for vtfp-runtime/include + vtfp-protocol/c-types include paths"
    if not has_resource and not has_fixture:
        recs["initial_setup"] = "Run `init-vtfp.py interview` then `init-vtfp.py codegen` to add vtfp.yaml"

    return DiscoverResult(
        project_root=str(root_path),
        rtos=rtos,
        rtos_version=rtos_ver,
        toolchain=toolchain,
        has_mklink_resource=has_resource,
        has_test_fixture=has_fixture,
        main_file=main_file,
        warnings=warnings,
        recommendations=recs,
    )
