# VTFP v1 — Virtual Test Fixture Protocol (范本)

A reusable reference implementation of a virtual test fixture protocol that lets AI/MCP clients control embedded MCUs over MKLink serial.

**Version 1.0** — see [CHANGELOG.md](CHANGELOG.md) for the full 4-milestone delivery log.

## What is this?

VTFP is a **shared-RAM protocol** between a PC (test agent) and an embedded MCU (device under test). Communication is mediated by the MKLink serial bridge. The protocol is **domain-neutral** — it carries generic command IDs and a generic state word. Domain semantics (genset, PLC, inverter, …) live in project-level `vtfp.yaml` configuration.

This 范本 provides a complete, runnable reference implementation with three layers:

| Layer | Module | What it provides |
|-------|--------|------------------|
| 1. Protocol | [vtfp-protocol/](vtfp-protocol/) | C headers, Python codec, JSON schema, cross-language tests, spec |
| 2. Firmware runtime | [vtfp-runtime/](vtfp-runtime/) | C state machine, 3 RTOS ports, codegen, host-pc tests |
| 3. Python SDK | [mklink-vtfp/](mklink-vtfp/) | High-level `VTFPAgent`, MKLink transport, MCP server codegen |
| 4. Scaffolding | [tools/init-vtfp.py](tools/init-vtfp.py) | `discover / interview / codegen / verify` subcommands |

Plus [samples/abstract-controller/](samples/abstract-controller/) (3 RTOS variants of a minimal demo) and [docs/vtfp/](docs/vtfp/) (architecture, porting, scaffolding, integration, glossary).

## Quick start

```bash
# 1. Discover your existing project
cd /path/to/your/project
python VTFP/tools/init-vtfp.py discover --root .

# 2. Generate vtfp.yaml via interactive interview
python VTFP/tools/init-vtfp.py interview --root .

# 3. Generate C dispatch + Python SDK + MCP server from vtfp.yaml
python VTFP/tools/init-vtfp.py codegen --config vtfp.yaml --out-dir generated/

# 4. Verify nothing broke
python VTFP/tools/init-vtfp.py verify --config vtfp.yaml
```

## Layout

```
VTFP/
├── README.md                        # this file
├── CHANGELOG.md                     # project-level changelog (M1-M4)
├── vtfp-protocol/                   # Layer 1: protocol (M1)
├── vtfp-runtime/                    # Layer 2: firmware runtime (M2)
├── mklink-vtfp/                     # Layer 3: Python SDK (M3)
├── samples/abstract-controller/     # Layer 4: 3 RTOS demo variants (M4)
├── tools/init-vtfp.py               # Layer 4: scaffolding tool (M4)
└── docs/vtfp/                       # Layer 4: documentation (M4)
```

## Tags

- `vtfp-v0.1.0` — protocol layer
- `vtfp-v0.2.0` — firmware runtime
- `vtfp-v0.3.0` — Python SDK
- `vtfp-v0.4.0` — scaffolding + samples + docs
- `vtfp-v1.0.0` — v1.0 release

## Tests (all passing)

| Suite | Count |
|-------|-------|
| M1 Python (`vtfp-protocol/py`) | 328 |
| M1 C (`vtfp-protocol/c-tests`) | 6 binaries |
| M2 C (`vtfp-runtime/tests`) | 4 tests, 75 assertions |
| M2 codegen (`vtfp-runtime/codegen`) | 2 tests |
| M3 Python (`mklink-vtfp`) | 19 |
| init-vtfp.py tests | 5 |
| **Total** | **~370+ test cases** |

## License

MIT (per-module — see individual `pyproject.toml` / SPDX headers)
