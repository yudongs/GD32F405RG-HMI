# INTEGRATION Recipes

5-line integration recipe for each supported RTOS. Drop into your existing project's main loop.

## RT-Thread

```c
#include <rtthread.h>
#include <vtfp/vtfp.h>

extern void vtfp_user_handlers_init(void);  // from vtfp_user_dispatch.c

static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

int main(void) {
    vtfp_init(&(vtfp_config_t){.header_base=0x20000000, .data_addr=(uint32_t)s_vtfp_data, .data_size=sizeof(s_vtfp_data), .safety_key=0xAA55AA55});
    vtfp_user_handlers_init();
    while (1) { vtfp_poll(); rt_thread_mdelay(20); }
}
```

## FreeRTOS

```c
#include <FreeRTOS.h>
#include <task.h>
#include <vtfp/vtfp.h>

extern void vtfp_user_handlers_init(void);

static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

int main(void) {
    vtfp_init(&(vtfp_config_t){.header_base=0x20000000, .data_addr=(uint32_t)s_vtfp_data, .data_size=sizeof(s_vtfp_data), .safety_key=0xAA55AA55});
    vtfp_user_handlers_init();
    for (;;) { vtfp_poll(); vTaskDelay(pdMS_TO_TICKS(20)); }
}
```

## Bare-Metal (Cortex-M SysTick)

```c
#include <vtfp/vtfp.h>
#include "board.h"

extern void vtfp_user_handlers_init(void);

static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);  /* 1ms tick; vtfp_port_bare.c provides handler */
    vtfp_init(&(vtfp_config_t){.header_base=0x20000000, .data_addr=(uint32_t)s_vtfp_data, .data_size=sizeof(s_vtfp_data), .safety_key=0xAA55AA55});
    vtfp_user_handlers_init();
    for (;;) vtfp_poll();
}
```

## Required build settings

All variants need these include paths and source files:

```makefile
# Include paths
CFLAGS += -Ivtfp-protocol/c-types
CFLAGS += -Ivtfp-runtime/include

# Source files (in addition to your application)
SRC += vtfp-runtime/src/vtfp_core.c
SRC += vtfp-runtime/src/vtfp_checksum.c
SRC += vtfp-runtime/src/vtfp_arm.c
SRC += vtfp-runtime/src/vtfp_dispatch.c
SRC += vtfp-runtime/codegen/../src/ports/vtfp_port_<your_rtos>.c
# vtfp_crc.c is in vtfp-protocol/c-types/ (extracted from M1-T11)
SRC += vtfp-protocol/c-types/vtfp_crc.c

# Your handlers
SRC += vtfp_user_dispatch.c

# Link with the RTOS kernel (your choice)
```

## PC-side test driver

After flashing, drive the MCU from a Python script:

```python
from mklink_vtfp import VTFPAgent
agent = VTFPAgent(port="COM5", config="vtfp.yaml")
agent.discover()
print(agent.query_info().product_id)
result = agent.invoke("TOGGLE_OUTPUT", param=0)  # ARM first if required
```

See [scaffolding-workflow.md](scaffolding-workflow.md) for the full PC-side workflow.
