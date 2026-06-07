/**
 * @file    vtfp_port_rtthread.c
 * @brief   RT-Thread port for VTFP runtime
 * @note    Build with RT-Thread headers on the include path.
 *          For host-pc unit tests, this file is excluded from the
 *          build (use vtfp_port_bare.c or the weak fallback instead).
 */

#include "vtfp_port.h"

#ifdef RT_THREAD

#include <rtthread.h>

uint32_t vtfp_port_now_ms(void) {
    return (uint32_t)rt_tick_get_millisecond();
}

void vtfp_port_sleep_ms(uint32_t ms) {
    rt_thread_mdelay(ms);
}

void vtfp_port_init(void) {
    /* RT-Thread doesn't need explicit port init — the kernel is already
     * running by the time user code calls vtfp_init. */
}

#else  /* !RT_THREAD — host-pc build fallback */

#include <time.h>

/* Weak fallback so vtfp_core.c's default vtfp_port_now_ms() can be used
 * in host-pc tests. The real RT-Thread version above overrides this
 * when RT_THREAD is defined. */
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

#endif /* RT_THREAD */
