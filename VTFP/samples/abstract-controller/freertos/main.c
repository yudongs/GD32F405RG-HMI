/**
 * @file    main.c
 * @brief   FreeRTOS main loop for the abstract-controller sample
 * @note    Uses vTaskDelay to yield between polls. The vtfp_port_freertos.c
 *            port provides vtfp_port_now_ms() backed by xTaskGetTickCount().
 */

#include <FreeRTOS.h>
#include <task.h>
#include <vtfp/vtfp.h>
#include <vtfp/vtfp_handlers.h>

extern void vtfp_user_handlers_init(void);

static uint8_t s_vtfp_data[1024] __attribute__((aligned(4)));

int main(void) {
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
        return -1;
    }
    vtfp_user_handlers_init();

    for (;;) {
        vtfp_poll();
        vTaskDelay(pdMS_TO_TICKS(cfg.poll_period_ms));
    }
    return 0;
}
