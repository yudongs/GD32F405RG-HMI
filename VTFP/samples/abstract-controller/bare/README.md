# Abstract Controller — Bare-Metal Sample

Same as the [RT-Thread sample](../rtthread/README.md) but without an RTOS. Uses SysTick for the 1ms tick and a busy-wait loop for the 20ms poll period. The vtfp_port_bare.c port provides `vtfp_port_now_ms()` backed by SysTick.

## Build (bare-metal + GCC)

```bash
# Include paths: -Ivtfp-protocol/c-types -Ivtfp-runtime/include -I<your-board>
# Source files:
#   vtfp-runtime/src/*.c
#   vtfp-runtime/src/ports/vtfp_port_bare.c
#   this directory: main.c + vtfp_user_dispatch.c
#   startup_<your-board>.c (CMSIS startup)
# Linker: STM32F405ZGTx_FLASH.ld (or your target's linker script)
```

## RTOS-specific differences

| Concern       | RT-Thread                             | Bare-Metal                            |
|---------------|---------------------------------------|---------------------------------------|
| Sleep call    | `rt_thread_mdelay(ms)`                | busy-wait loop + `__NOP()`             |
| Tick source   | `rt_tick_get_millisecond()`            | SysTick (1ms)                         |
| Port file     | `vtfp_port_rtthread.c`                | `vtfp_port_bare.c`                    |

Everything else (vtfp.yaml, vtfp_user_dispatch.c, PC-side scripts) is identical.

## Power consumption

The bare-metal sample uses a busy-wait loop. For power-sensitive applications, replace the wait with `__WFI()` (Wait For Interrupt), which the SysTick will wake on every 1ms tick.

## See also

See the [RT-Thread sample README](../rtthread/README.md) for the full integration story.
