/**
 * @file    main.c
 * @brief   RT-Thread main loop for the abstract-controller sample
 * @note    This is the integration point: call vtfp_init() once,
 *            register user handlers, then call vtfp_poll() in a loop.
 *
 *            For a real RT-Thread build, this file would be compiled
 *            with the RT-Thread headers on the include path and the
 *            vtfp-runtime linked in. The vtfp_port_rtthread.c port
 *            provides vtfp_port_now_ms() backed by rt_tick_get_millisecond().
 */

#include <rtthread.h>
#include <vtfp/vtfp.h>
#include <vtfp/vtfp_handlers.h>

/* The user_dispatch.c file declares this */
extern void vtfp_user_handlers_init(void);

/* === Static data buffer (.vtfp_data) === */
static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

int main(void) {
    /* Initialize the VTFP runtime pointing at the static data buffer.
     * Note: data_addr is at header_base + sizeof(vtfp_header_t) = 0x20000000 + 44 = 0x2000002C.
     * In a real RT-Thread build, header_base would typically be 0x20000000 and
     * data_addr would be placed after the header (e.g., in a named SRAM section). */
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
        rt_kprintf("vtfp_init failed\n");
        return -1;
    }

    /* Register user-defined handlers */
    vtfp_user_handlers_init();

    rt_kprintf("VTFP v1 abstract-controller sample started\n");
    rt_kprintf("  vtfp runtime: %s\n", vtfp_version());
    rt_kprintf("  poll period:  %u ms\n", cfg.poll_period_ms);
    rt_kprintf("  ARM timeout:  %u ms\n", cfg.arm_timeout_ms);
    rt_kprintf("Waiting for commands...\n");

    /* Main loop: poll every 20ms (matches poll_period_ms) */
    while (1) {
        vtfp_poll();
        rt_thread_mdelay(cfg.poll_period_ms);
    }
    return 0;
}
