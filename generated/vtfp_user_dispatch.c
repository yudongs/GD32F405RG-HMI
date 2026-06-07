/**
 * @file    vtfp_user_dispatch.c
 * @brief   User-defined VTFP command handlers (gd32f405rg-hmi)
 * @detail  Bridges VTFP commands to the HMI's existing APP layer:
 *            - 0x01 KEY_PRESS         -> menu_navigate()
 *            - 0x02 SET_DISPLAY_MODE  -> LCD_SetDisplayMode()
 *            - 0x03 DUMP_FRAMEBUFFER  -> memcpy from lcd_get_fb()
 *            - 0x04 QUERY_STATE       -> pack cursor + last_key + disp_mode + uptime
 *
 *          Source vtfp.yaml: vtfp.yaml
 *          Project: gd32f405rg-hmi (GD32F405RG-HMI-1.0)
 */
#include "vtfp_compat.h"   /* AC5 _Static_assert polyfill + VTFP_R_* result codes */
#include <vtfp/vtfp.h>
#include <vtfp/vtfp_handlers.h>

#include "lcd_st7586.h"   /* lcd_get_fb(), lcd_get_fb_size() */
#include "lcd_test.h"     /* LCD_SetDisplayMode(), LCD_GetDisplayMode() */
#include "menu.h"         /* menu_navigate(), menu_get_cursor_index(), menu_set_last_key() */
#include "key.h"          /* key_event_t, KEY_EVENT_* */
#include "FreeRTOS.h"     /* xTaskGetTickCount */
#include "task.h"

#include <string.h>       /* memcpy */

/* === BEGIN user handler implementations (editable) === */

/**
 * @brief  0x01 KEY_PRESS — 注入一个按键事件到 menu 导航器
 * @param  req->param[7:0] = KEY_EVENT_*: 1=UP 2=DOWN 3=ENTER 4=ESC
 */
static int32_t my_key_press_handler(const vtfp_request_t *req, vtfp_response_t *resp)
{
    (void)resp;
    uint8_t k = (uint8_t)(req->param & 0xFFu);
    /* 只接受 4 个短按事件(忽略 NONE 和 LONG 变体) */
    if (k != KEY_EVENT_UP && k != KEY_EVENT_DOWN &&
        k != KEY_EVENT_ENTER && k != KEY_EVENT_ESC) {
        return (int32_t)VTFP_R_PARAM_RANGE;
    }
    menu_set_last_key((key_event_t)k);
    menu_navigate((key_event_t)k);
    return 0;
}

/**
 * @brief  0x02 SET_DISPLAY_MODE — 切换 LCD 测试显示模式
 * @param  req->param[7:0] = 0..6 (DISPLAY_MODE_AUTO..COLORFILL)
 */
static int32_t my_set_display_mode_handler(const vtfp_request_t *req, vtfp_response_t *resp)
{
    (void)resp;
    uint8_t m = (uint8_t)(req->param & 0xFFu);
    if (m >= (uint8_t)DISPLAY_MODE_COUNT) {
        return (int32_t)VTFP_R_PARAM_RANGE;
    }
    LCD_SetDisplayMode((display_mode_t)m);
    return 0;
}

/**
 * @brief  0x03 DUMP_FRAMEBUFFER — 复制 4800 字节 framebuffer 到 resp->data
 *         布局:240 列 × 160 行 × 1bpp,3 像素 packed 进 1 字节(经 ChangeTab 编码后)
 *         PC 端需知此打包方式才能正确解码。
 */
static int32_t my_dump_framebuffer_handler(const vtfp_request_t *req, vtfp_response_t *resp)
{
    (void)req;
    uint32_t fb_size = lcd_get_fb_size();
    if (resp->data_len < fb_size) {
        return (int32_t)VTFP_R_BUF_OVERFLOW;
    }
    const uint8_t *fb = lcd_get_fb();
    if (fb == NULL) {
        return (int32_t)VTFP_R_NOT_INIT;  /* framebuffer 未初始化 */
    }
    memcpy(resp->data, fb, fb_size);
    return 0;
}

/**
 * @brief  0x04 QUERY_STATE — 返回 16 字节 LE
 *         [0..3]   cursor_index (u32) — 当前焦点在同级列表中的位置
 *         [4..7]   last_key     (u32) — 最近一次按键事件(KEY_EVENT_* 枚举值)
 *         [8..11]  disp_mode    (u32) — 当前显示模式(DISPLAY_MODE_* 枚举值)
 *         [12..15] uptime_ms    (u32) — 系统启动到现在的毫秒数
 */
static int32_t my_query_state_handler(const vtfp_request_t *req, vtfp_response_t *resp)
{
    (void)req;
    if (resp->data_len < 16u) {
        return (int32_t)VTFP_R_BUF_OVERFLOW;
    }
    uint32_t cursor    = menu_get_cursor_index();
    uint32_t last_key  = (uint32_t)menu_get_last_key();
    uint32_t disp_mode = (uint32_t)LCD_GetDisplayMode();
    uint32_t uptime_ms = (uint32_t)((uint64_t)xTaskGetTickCount() * 1000u / configTICK_RATE_HZ);

    /* Cortex-M4 是 little-endian,直接 memcpy u32 即可 */
    uint8_t *p = resp->data;
    memcpy(p + 0,  &cursor,    4);
    memcpy(p + 4,  &last_key,  4);
    memcpy(p + 8,  &disp_mode, 4);
    memcpy(p + 12, &uptime_ms, 4);
    return 0;
}

/* === END user handler implementations === */


/**
 * @brief  Register all user-defined handlers. Call from VTFP_Task after
 *         vtfp_init() and before entering the poll loop.
 */
void vtfp_user_handlers_init(void)
{
    vtfp_register_handler(0x01, my_key_press_handler,        VTFP_ARM_REQUIRED);
    vtfp_register_handler(0x02, my_set_display_mode_handler, VTFP_ARM_REQUIRED);
    vtfp_register_handler(0x03, my_dump_framebuffer_handler, VTFP_ARM_REQUIRED);
    vtfp_register_handler(0x04, my_query_state_handler,      VTFP_ARM_REQUIRED);
}
