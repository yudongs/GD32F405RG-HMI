# VTFP v1范本 — Documentation

A virtual test fixture protocol that lets AI/MCP clients control embedded MCUs over MKLink serial.

## Index

| Doc                                      | Purpose                                            |
|------------------------------------------|----------------------------------------------------|
| [architecture.md](architecture.md)       | 3-layer architecture (protocol / runtime / SDK)   |
| [porting-guide.md](porting-guide.md)     | How to add a new RTOS port or domain                |
| [scaffolding-workflow.md](scaffolding-workflow.md) | `init-vtfp.py` tutorial (discover / interview / codegen / verify) |
| [INTEGRATION.md](INTEGRATION.md)         | 5-line integration recipe per RTOS                  |
| [glossary.md](glossary.md)               | Terminology (VTFP / handle / port / codegen / SoT)  |

## Source of truth

- Protocol spec: [`../../vtfp-protocol/SPEC.md`](../../vtfp-protocol/SPEC.md)
- C runtime API: [`../../vtfp-runtime/include/vtfp/vtfp.h`](../../vtfp-runtime/include/vtfp/vtfp.h)
- Python SDK API: `mklink_vtfp.VTFPAgent` (see [mklink-vtfp README](../../mklink-vtfp/README.md))
- Scaffolding tool: [`../../tools/init-vtfp.py`](../../tools/init-vtfp.py)

## Tags

- `vtfp-v0.1.0` — protocol layer (M1)
- `vtfp-v0.2.0` — firmware runtime (M2)
- `vtfp-v0.3.0` — Python SDK (M3)
- (M4 = scaffolding + samples; tag in M4-Final)
