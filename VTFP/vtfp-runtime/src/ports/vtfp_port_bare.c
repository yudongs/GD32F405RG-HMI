/**
 * @file    vtfp_port_bare.c
 * @brief   Bare-metal port for VTFP runtime
 * @note    Uses SysTick on Cortex-M (or equivalent on other MCUs).
 *          For host-pc unit tests, the SysTick code is excluded and
 *          a fake-time fallback is provided.
 */

#include "vtfp_port.h"

#if defined(__ARM_ARCH) || defined(__CORTEX_M) || defined(STM32F405xx) || defined(STM32F40_41xxx)

/* Cortex-M SysTick-based timing */
#include "board.h"  /* user-provided: SystemCoreClock */

static volatile uint32_t s_tick_ms = 0;

void SysTick_Handler(void) {
    s_tick_ms++;
}

uint32_t vtfp_port_now_ms(void) {
    return s_tick_ms;
}

void vtfp_port_sleep_ms(uint32_t ms) {
    /* Crude busy-wait. Real implementations should use a low-power
     * wait or WFI instruction. */
    uint32_t target = s_tick_ms + ms;
    while (s_tick_ms < target) {
        __WFI();  /* wait for next interrupt */
    }
}

void vtfp_port_init(void) {
    /* Configure SysTick for 1ms tick. User must call SystemCoreClockUpdate()
     * before this. */
    extern uint32_t SystemCoreClock;
    SysTick_Config(SystemCoreClock / 1000U);
    s_tick_ms = 0;
}

#else  /* host-pc build fallback */

#include <time.h>

__attribute__((weak)) uint32_t vtfp_port_now_ms(void) {
    static uint32_t fake = 0;
    return fake++;
}

void vtfp_port_sleep_ms(uint32_t ms) {
    (void)ms;  /* no-op on host */
}

void vtfp_port_init(void) {
    /* no-op */
}

#endif /* __ARM_ARCH */
