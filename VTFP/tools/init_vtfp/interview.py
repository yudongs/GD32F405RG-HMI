"""Interview subcommand: interactive Q&A to generate vtfp.yaml."""

from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional, Dict, Any

import yaml

from . import state


DEFAULT_PROJECT = "default"


@dataclass
class InterviewAnswers:
    project_name: str = "my-controller"
    product_id: str = "MC-1.0"
    model_code: int = 0
    rtos: str = "rt-thread"             # rt-thread | freertos | bare
    tick_ms: int = 20
    arm_timeout_ms: int = 2000
    safety_key: str = "0xAA55AA55"
    auto_disarm: bool = True
    header_base: str = "0x20000000"
    data_buffer_size: int = 1024
    alignment: int = 4
    commands: list = field(default_factory=list)

    def to_yaml(self) -> str:
        # Build a vtfp.yaml-shaped dict
        cfg = {
            "project": {
                "name": self.project_name,
                "product_id": self.product_id,
                "model_code": self.model_code,
            },
            "protocol": {
                "version": "0.1.0",
                "features": [],
            },
            "memory": {
                "header_base": self.header_base,
                "data_buffer_size": self.data_buffer_size,
                "alignment": self.alignment,
            },
            "safety": {
                "arm_timeout_ms": self.arm_timeout_ms,
                "safety_key": self.safety_key,
                "auto_disarm": self.auto_disarm,
            },
            "rtos": {
                "type": self.rtos,
                "tick_ms": self.tick_ms,
            },
            "commands": self.commands,
        }
        return yaml.safe_dump(cfg, allow_unicode=True, sort_keys=False, default_flow_style=False)


def _ask(question: str, default: str) -> str:
    """Print a question with default; read a line; return the answer (or default if empty)."""
    suffix = f" [{default}]: " if default else ": "
    try:
        ans = input(question + suffix).strip()
    except EOFError:
        ans = ""
    return ans if ans else default


def _ask_int(question: str, default: int) -> int:
    s = _ask(question, str(default))
    try:
        return int(s)
    except ValueError:
        return default


def _ask_bool(question: str, default: bool) -> bool:
    s = _ask(question, "y" if default else "n").lower()
    if s in ("y", "yes", "true", "1"):
        return True
    if s in ("n", "no", "false", "0"):
        return False
    return default


def run(root: str, resume: bool = False) -> InterviewAnswers:
    """Run the interview. Returns the answers object (caller writes yaml)."""
    saved = state.load(root, DEFAULT_PROJECT) if resume else None

    def get(key: str, default: Any) -> Any:
        if saved and key in saved:
            return saved[key]
        return default

    print("=== VTFP v1 Project Setup Interview ===")
    print("Press Enter to accept defaults.\n")

    ans = InterviewAnswers()
    ans.project_name    = _ask("Project name", get("project_name", ans.project_name))
    ans.product_id      = _ask("Product ID (e.g. MC-1.0)", get("product_id", ans.product_id))
    ans.model_code      = _ask_int("Model code (uint16)", int(get("model_code", ans.model_code)))
    ans.rtos            = _ask("RTOS type (rt-thread / freertos / bare)", get("rtos", ans.rtos))
    ans.tick_ms         = _ask_int("RTOS tick period (ms)", int(get("tick_ms", ans.tick_ms)))
    ans.arm_timeout_ms  = _ask_int("ARM timeout (ms)", int(get("arm_timeout_ms", ans.arm_timeout_ms)))
    ans.safety_key      = _ask("Safety key (hex32, e.g. 0xAA55AA55)", get("safety_key", ans.safety_key))
    ans.auto_disarm     = _ask_bool("Auto-disarm after each dangerous command", bool(get("auto_disarm", ans.auto_disarm)))
    ans.header_base     = _ask("Header base address (hex32)", get("header_base", ans.header_base))
    ans.data_buffer_size = _ask_int("Data buffer size (bytes)", int(get("data_buffer_size", ans.data_buffer_size)))
    ans.alignment       = _ask_int("Data buffer alignment (1/2/4/8)", int(get("alignment", ans.alignment)))

    print("\nNow define your commands (user-defined, IDs 0x01..0x0F):")
    while True:
        cmd_name = _ask("Command name (UPPER_SNAKE_CASE) or empty to stop", "")
        if not cmd_name:
            break
        cmd_id = _ask_int("  Command ID (0x01..0x0F)", len(ans.commands) + 1)
        desc = _ask("  Description", "")
        requires_arm = _ask_bool("  Requires ARM?", False)
        ans.commands.append({
            "id": cmd_id,
            "name": cmd_name,
            "description": desc,
            "direction": "pc_to_mcu",
            "requires_arm": requires_arm,
        })
        print(f"  + Added {cmd_name} (0x{cmd_id:02X}, arm={requires_arm})")

    # Save state for resume
    state.save(root, asdict(ans), DEFAULT_PROJECT)
    print(f"\n(Saved state to {state.state_path(root, DEFAULT_PROJECT)})")

    return ans
