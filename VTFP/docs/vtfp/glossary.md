# Glossary

| Term             | Definition                                                                                          |
|------------------|-----------------------------------------------------------------------------------------------------|
| ARM              | Safety protocol state. When "armed", dangerous commands may execute for `arm_timeout_ms` (default 2000ms). |
| Auto-disarm      | After a dangerous command executes, automatically clear the ARM state (configurable, default true). |
| Codegen          | Code generation. Reading a `vtfp.yaml` and producing C dispatch + Python SDK + MCP server files. |
| Command ID       | 8-bit identifier in `header.command`. Three partitions: user (0x01-0x0F), standard (0x10-0x1F), safety (0xF0-0xFF). |
| Data segment     | `.vtfp_data` SRAM region (default 1KB) used for command/response payloads. |
| Dispatch         | The act of looking up a command ID in a handler table and invoking the matching C function. |
| DMR              | (Not a VTFP term; this is a placeholder.) |
| E2E              | End-to-end test.                                                                                     |
| Handler          | A user-defined C function with signature `int32_t (*)(const vtfp_request_t *req, vtfp_response_t *resp)`. |
| Header           | The 44-byte `vtfp_header_t` struct in shared RAM.                                                    |
| MCP              | Model Context Protocol. The AI-side wire protocol for tool invocation.                              |
| MKLink           | The serial-bridge tool that exposes `cmd.read_ram` / `cmd.write_ram` / `cmd.dump_memory` to a PC. |
| Mock bridge      | A test double for `MKLinkSerialBridge` that simulates MCU responses without real hardware.        |
| Payload          | The variable-length data in the `.vtfp_data` segment accompanying a command.                       |
| Poll period      | How often the main loop calls `vtfp_poll()`. Typical: 20ms (matches RT-Thread default tick).     |
| Port             | The RTOS-specific layer that provides `vtfp_port_now_ms()` and `vtfp_port_sleep_ms()`.            |
| Result code      | 8-bit code in `header.result`. 0 = OK, non-zero = error (see `VTFP_R_*` in `vtfp_result.h`). |
| Safety key       | 32-bit value the PC must send to ARM. Default `0xAA55AA55`. Configurable.                          |
| Scaffolding      | The act of generating a project's VTFP integration from `vtfp.yaml` via `init-vtfp.py`.         |
| SDK              | Software Development Kit. In VTFP, the Python `mklink-vtfp` package.                                |
| Shared RAM       | The SRAM region starting at `vtfp.yaml:memory.header_base` (default 0x20000000). Both PC and MCU read/write this. |
| SoT              | Source of Truth. The single YAML file (`vtfp.yaml`) that defines the project's VTFP config.       |
| Tick             | RTOS periodic interrupt, typically 1ms. The runtime uses this to enforce ARM timeouts.         |
| Transaction      | A single command/response exchange, identified by `header.seq`.                                    |
| VTFP             | Virtual Test Fixture Protocol v1. The protocol this document describes.                             |
| Wire format      | The exact byte-level layout of the 44-byte header in shared RAM. Must match between C and Python. |

## Acronyms

- **MCU** — Microcontroller Unit (the embedded device)
- **PC**  — Personal Computer (the host running the test agent)
- **PCP** — not used
- **RTOS** — Real-Time Operating System (RT-Thread, FreeRTOS, Zephyr, ...)
- **SRAM** — Static RAM (the MCU's volatile memory)
- **STM32** — STMicroelectronics 32-bit microcontroller family (e.g., STM32F405)
- **CRC** — Cyclic Redundancy Check (we use CRC-32 IEEE 802.3)
- **MSB/LSB** — Most/Least Significant Bit/Byte (we use little-endian on the wire)
- **NUL** — The byte `0x00` used to terminate C strings in QUERY_INFO response
