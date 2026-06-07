/**
 * @file    vtfp_port_freertos.c
 * @brief   FreeRTOS port for VTFP runtime
 * @note    Build with FreeRTOS headers on the include path.
 *          For host-pc unit tests, this file is excluded.
 */

#include "vtfp_port.h"

#ifdef FREERTOS_CONFIG_H  /* FreeRTOSConfig.h was included somewhere upstream */

#include "FreeRTOS.h"
#include "task.h"

uint32_t vtfp_port_now_ms(void) {
    return (uint32_t)((uint64_t)xTaskGetTickCount() * 1000U / configTICK_RATE_HZ);
}

void vtfp_port_sleep_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void vtfp_port_init(void) {
    /* no-op; FreeRTOS scheduler is already running */
}

#else  /* !FREERTOS — host-pc build fallback */

__attribute__((weak)) uint32_t vtfp_port_now_ms(void) {
    static uint32_t fake = 0;
    return fake++;
}

void vtfp_port_sleep_ms(uint32_t ms) { (void)ms; }
void vtfp_port_init(void) {}

#endif /* FREERTOS_CONFIG_H */
