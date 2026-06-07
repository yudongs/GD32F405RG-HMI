# Abstract Controller — FreeRTOS Sample

Same as the [RT-Thread sample](../rtthread/README.md) but with FreeRTOS as the host RTOS. The user-dispatch handlers (`vtfp_user_dispatch.c`) are byte-for-byte identical to the RT-Thread version.

## Build (FreeRTOS + GCC)

```bash
# Include paths: -Ivtfp-protocol/c-types -Ivtfp-runtime/include
# Source files:
#   vtfp-runtime/src/*.c
#   vtfp-runtime/src/ports/vtfp_port_freertos.c
#   this directory: main.c + vtfp_user_dispatch.c
#   FreeRTOS kernel + portable layer (your choice)
```

## RTOS-specific differences

| Concern       | RT-Thread                             | FreeRTOS                              |
|---------------|---------------------------------------|---------------------------------------|
| Sleep call    | `rt_thread_mdelay(ms)`                | `vTaskDelay(pdMS_TO_TICKS(ms))`       |
| Tick source   | `rt_tick_get_millisecond()`            | `xTaskGetTickCount() * 1000 / configTICK_RATE_HZ` |
| Port file     | `vtfp_port_rtthread.c`                | `vtfp_port_freertos.c`                |

Everything else (vtfp.yaml, vtfp_user_dispatch.c, PC-side scripts) is identical.

## See also

See the [RT-Thread sample README](../rtthread/README.md) for the full integration story.
