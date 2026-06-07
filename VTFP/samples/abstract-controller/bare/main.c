/**
 * @file    main.c
 * @brief   Bare-metal main loop for the abstract-controller sample
 * @note    Uses SysTick for 1ms tick. vtfp_port_bare.c provides
 *            vtfp_port_now_ms() backed by SysTick_Handler incrementing
 *            a static counter.
 */

#include <vtfp/vtfp.h>
#include <vtfp/vtfp_handlers.h>
#include "board.h"   /* user-provided: SystemCoreClock */

extern void vtfp_user_handlers_init(void);

static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

/* SysTick_Handler is provided by vtfp_port_bare.c (increments a static counter) */

int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);  /* 1ms tick */

    vtfp_config_t cfg = {
        .header_base     = 0x20000000,
        .data_addr       = (uint32_t)(uintptr_t)s_vtfp_data,
        .data_size       = sizeof(s_vtfp_data),
        .poll_period_ms  = 20,
        .arm_timeout_ms  = 2000,
        .safety_key      = 0xAA55AA55,
        .auto_disarm     = true,
    };
    if (vtfp_init(&cfg) != 0) {
        while (1) { __WFI(); }  /* spin */
    }
    vtfp_user_handlers_init();

    for (;;) {
        vtfp_poll();
        /* Crude 20ms sleep — real impl should use a low-power wait */
        for (volatile uint32_t i = 0; i < (SystemCoreClock / 1000 / 50); i++) {
            __NOP();
        }
    }
    return 0;
}
