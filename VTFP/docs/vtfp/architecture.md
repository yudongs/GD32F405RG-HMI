# VTFP v1 Architecture

## 3-layer model

```
┌─────────────────────────────────────────────────────────────┐
│                  vtfp.yaml  (project SoT)                    │
│  - protocol version / MCU / RTOS 适配                      │
│  - command set / ARM policy / memory layout                 │
└─────────────────────────────────────────────────────────────┘
        │                                       │
        ↓ codegen                               ↓ codegen
┌──────────────────────┐                ┌──────────────────────┐
│  固件侧 (C)           │                │  主机侧 (Python)      │
│  vtfp-runtime/       │                │  mklink-vtfp/         │
│  ├─ vtfp_core.c      │                │  ├─ sdk.py            │
│  ├─ vtfp_dispatch.c  │  ← codegen     │  ├─ transport/        │
│  ├─ port_rtthread.c  │                │  └─ codegen.py        │
│  ├─ port_freertos.c  │                │                      │
│  └─ port_bare.c      │                │  mcp_vtfp_server.py   │
└──────────────────────┘                │  (codegen 自 yml)    │
        ↑                               └──────────────────────┘
        └──── MKLink shared RAM ────────┘
            (SRAM 0x20000000, header + data)
```

## Layer 1: Protocol (`vtfp-protocol/`)

**Deliverable:** M1, tag `vtfp-v0.1.0`.

- C header type definitions (zero-dependency, host-pc compilable)
- Python package `vtfp_proto` (constants, header pack/unpack, CRC-32)
- JSON Schema for project config (`vtfp.schema.json`)
- Cross-language test: Python emits a `header.bin`, C reads and verifies every field

The protocol is the **stable contract** between MCU and PC. Everything else is implementation.

## Layer 2: Firmware Runtime (`vtfp-runtime/`)

**Deliverable:** M2, tag `vtfp-v0.2.0`.

- Public API: `vtfp.h`, `vtfp_types.h`, `vtfp_handlers.h`
- Core: `vtfp_core` (poll loop, dispatch, ARM enforcement, result writing)
- Checksum: `vtfp_checksum` (CRC-32 over header bytes [0..35])
- ARM: `vtfp_arm` (state machine with timeout and auto-disarm)
- Dispatch: `vtfp_dispatch` (standard commands QUERY_INFO, QUERY_STATE)
- Port layer: 3 ports (RT-Thread, FreeRTOS, bare-metal Cortex-M SysTick)

The runtime is RTOS-agnostic at the API surface. The port layer provides `vtfp_port_now_ms()` and `vtfp_port_sleep_ms()`.

## Layer 3: Python SDK (`mklink-vtfp/`)

**Deliverable:** M3, tag `vtfp-v0.3.0`.

- `VTFPAgent` — high-level API (discover, query_info, invoke, arm, disarm)
- Transport: `MKLinkSerialBridge` (pyserial wrapper) + dump_memory stub
- Codegen: `generate_mcp_server` (FastMCP) + `generate_c_dispatch` (delegates to vtfp-runtime)

The SDK is the layer that AI/MCP clients interact with. Each project gets a generated MCP server that exposes the project's user commands as `@mcp.tool()` functions.

## Layer 4: Scaffolding + Samples (`tools/init-vtfp.py` + `samples/`)

**Deliverable:** M4 (current).

- `init-vtfp.py discover` — scan a project, detect RTOS, propose defaults
- `init-vtfp.py interview` — interactive Q&A, writes `vtfp.yaml`
- `init-vtfp.py codegen` — read `vtfp.yaml`, write C dispatch + Python SDK + MCP server
- `init-vtfp.py verify` — run protocol-level tests (no hardware)
- `samples/abstract-controller/{rtthread,freertos,bare}/` — minimal demo projects

The scaffolding tool is the **entry point** for new projects. It encodes the "do this first" workflow.

## Wire format (recap)

The shared RAM layout:
```
0x20000000 ┌──────────────────────────┐
           │  vtfp_header_t (44B)     │  control struct
0x2000002C ├──────────────────────────┤
           │  .vtfp_data (≥1KB)        │  command/response payload
           └──────────────────────────┘
```

The 44-byte header has 12 fields (magic, version, features, command, result, seq, param, data_addr, data_len, flags, checksum, reserved). The Python `vtfp_proto.VTFHeader` and the C `vtfp_header_t` MUST match byte-for-byte (verified by M1's cross-language test).

## ARM safety protocol

Commands that have `requires_arm: true` in `vtfp.yaml` MUST be preceded by a `VTFP_CMD_ARM` (cmd 0xFE) with the correct safety key. The ARM window is 2 seconds (configurable) and auto-disarms after each dangerous command (configurable).

See [`../../vtfp-protocol/SPEC.md`](../../vtfp-protocol/SPEC.md) §4 for the full ARM protocol.
