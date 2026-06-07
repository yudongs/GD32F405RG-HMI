# Changelog

## 1.0.0 — 2026-06-05 — 范本 v1.0 交付

**First complete delivery of the Virtual Test Fixture Protocol (VTFP) v1范本.**

4 milestones, 28+ commits on `feat/vtfp-m1-protocol`, 4 tags.

### M1 — Protocol Layer (tag `vtfp-v0.1.0`)

- 5 C headers: `vtfp_header.h` (44-byte packed struct), `vtfp_command.h` (partition + standard + safety cmds), `vtfp_result.h` (result code namespace), `vtfp_safety.h` (ARM primitives), `vtfp_crc.h` + `vtfp_crc.c` (CRC-32 IEEE 802.3)
- Python package `vtfp_proto` (M1-T8..T10): constants, header pack/unpack, CRC-32 — 326 tests + 2 cross-language = 328 passing
- SPEC.md (179 lines, 7 sections)
- `vtfp.schema.json` (Draft-07) + sample yaml + schema test

### M2 — Firmware Runtime (tag `vtfp-v0.2.0`)

- Public API: `vtfp.h`, `vtfp_types.h`, `vtfp_handlers.h`
- Core: `vtfp_core` (poll loop, dispatch, ARM enforcement, result writing)
- CRC-32 wrapper, ARM state machine, standard command dispatch (QUERY_INFO, QUERY_STATE)
- 3 RTOS ports: RT-Thread (full), FreeRTOS (full), bare-metal (Cortex-M SysTick)
- 4 host-pc C tests (75 assertions), all pass
- Codegen: `gen_c_dispatch.py` reads vtfp.yaml, emits C dispatch source

### M3 — Python SDK (tag `vtfp-v0.3.0`)

- `mklink-vtfp` package: transport (MKLinkSerialBridge), SDK (VTFPAgent high-level), codegen
- 17 SDK methods: discover, query_info, query_state, arm, disarm, invoke
- 8 exception types mapped to specific failure modes
- Codegen: `generate_mcp_server` (FastMCP) + `generate_c_dispatch` (delegates to vtfp-runtime)
- 19 pytest tests, all pass
- Bridge bug fixes during M3-T4 (RESULT_EXCEPTIONS typo, _read_u32 byte order, port_busy exception broadening)

### M4 — Scaffolding + Samples + Documentation (tag `vtfp-v0.4.0`)

- `tools/init-vtfp.py`: 4 subcommands (discover / interview / codegen / verify)
- `samples/abstract-controller/{rtthread,freertos,bare}/` — 3 RTOS variants of the same demo
- 6 documentation files in `docs/vtfp/`: architecture, porting-guide, scaffolding-workflow, INTEGRATION, glossary, README
- 5 init-vtfp.py tests, all pass

### Test totals (post-M4)

| Suite                              | Count    | Status     |
|------------------------------------|----------|------------|
| M1 Python (`vtfp-protocol/py`)     | 328      | ✅ pass    |
| M1 C (`vtfp-protocol/c-tests`)      | 6 tests  | ✅ pass    |
| M2 C (`vtfp-runtime/tests`)         | 4 tests, 75 assertions | ✅ pass |
| M2 codegen (`vtfp-runtime/codegen`)| 2 tests  | ✅ pass    |
| M3 Python (`mklink-vtfp`)          | 19       | ✅ pass    |
| init-vtfp.py tests                 | 5        | ✅ pass    |
| **Total**                           | **~370+ test cases** | ✅ all pass |

### Known limitations (out of scope for v1.0)

- `make test` end-to-end on Windows MSYS2 fails due to shell path resolution (env issue, not code defect). Individual test binaries pass.
- vtfp_result.h ↔ vtfp_header.h dual namespace (documented in SPEC.md §3.2; canonical is `VTFP_R_*`).
- vtfp-runtime CHANGELOG 0.1.0 entry doesn't mention all delivered modules (M2-Final expanded it).
- Test gaps: exact-value assertions for some commands/result codes (covered indirectly by partition tests).
- Optional SPEC.md polish: forward-compat definition, RESET type qualifier.
