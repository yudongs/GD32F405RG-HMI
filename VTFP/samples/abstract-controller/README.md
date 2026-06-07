# Abstract Controller — Sample

A minimal, domain-neutral VTFP v1 application demonstrating end-to-end integration across three RTOS environments.

## Variants

| Directory | RTOS          | Sleep call              | Tick source              |
|-----------|---------------|-------------------------|--------------------------|
| `rtthread/` | RT-Thread    | `rt_thread_mdelay(ms)`  | `rt_tick_get_millisecond()` |
| `freertos/` | FreeRTOS     | `vTaskDelay(pdMS_TO_TICKS(ms))` | `xTaskGetTickCount() * 1000 / configTICK_RATE_HZ` |
| `bare/`     | Bare-metal   | busy-wait + `__NOP()`   | SysTick (1ms)            |

All three variants implement the **same 3 user commands** and **same internal state** — only `main.c` differs (and the vtfp.yaml `rtos.type` field). The shared `vtfp_user_dispatch.c` is byte-for-byte identical across the three.

## Commands

| ID  | Name           | ARM   | Description                              |
|-----|----------------|-------|------------------------------------------|
| 0x01 | QUERY_STATE    | No    | Read 8-byte state word                   |
| 0x02 | SET_PARAM      | No    | Set `params[idx]` to `value`             |
| 0x03 | TOGGLE_OUTPUT  | Yes   | Toggle bit `idx` in `output_bits`        |

## Internal state

```c
static struct {
    uint32_t params[4];      // SET_PARAM storage
    uint32_t state_word;     // (unused; QUERY_STATE packs button_presses + params[0])
    uint8_t  output_bits;    // TOGGLE_OUTPUT bits 0..7
    uint32_t button_presses; // counter, incremented by TOGGLE_OUTPUT
} s_state;
```

## PC-side driver

```python
from mklink_vtfp import VTFPAgent
agent = VTFPAgent(port="COM5", config="rtthread/vtfp.yaml")
agent.discover()
state = agent.invoke("QUERY_STATE").data.hex()  # 8 bytes
agent.invoke("SET_PARAM", param=0x00000001)
agent.arm()
agent.invoke("TOGGLE_OUTPUT", param=0x00)  # bit 0
```

## See also

- [`../../vtfp-protocol/SPEC.md`](../../vtfp-protocol/SPEC.md) — protocol spec
- [`../../vtfp-runtime/`](../../vtfp-runtime/) — C runtime
- [`../../mklink-vtfp/`](../../mklink-vtfp/) — Python SDK
- [`../../tools/init-vtfp.py`](../../tools/init-vtfp.py) — scaffolding tool
