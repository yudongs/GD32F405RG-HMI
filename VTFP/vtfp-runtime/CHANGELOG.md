# Changelog

## 0.1.0 — 2026-06-05
- Initial vtfp-runtime (M2)
- Public API: vtfp.h, vtfp_types.h, vtfp_handlers.h
- Core types: vtfp_config_t, vtfp_request_t, vtfp_response_t, vtfp_handler_fn, vtfp_arm_policy_t
- Core implementation: vtfp_core (poll loop, dispatch, ARM enforcement, result writing)
- CRC-32 wrapper: vtfp_checksum (over vtfp-protocol's vtfp_crc32)
- ARM state machine: vtfp_arm (timeout, auto-disarm)
- Standard command dispatch: vtfp_dispatch (QUERY_INFO, QUERY_STATE)
- Port layer: RT-Thread (full), FreeRTOS (full), bare-metal (Cortex-M SysTick)
- Host-pc unit tests: 4 binaries, 64 OK assertions (checksum, arm, dispatch, core)
- Codegen: gen_c_dispatch.py reads vtfp.yaml, emits C dispatch source with stubs
- M1 cross-language tests still pass (328 Python + 5 C tests)
