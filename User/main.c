#include "main.h"
#include "SEGGER_RTT.h"
#include "lcd_st7586.h"
#include "lcd_test.h"
#include "key.h"
#include "menu.h"

/* FreeRTOS headers */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#ifdef USE_VTFP
#include "vtfp_compat.h"        /* AC5 _Static_assert polyfill + VTFP_R_* */
#include "vtfp/vtfp.h"
#include "vtfp_user_dispatch.h"   /* vtfp_user_handlers_init */

/* VTFP shared-RAM region: 44 B header + 5108 B data buffer.
 * Placed in the .vtfp section (see Project/Objects/GD32F405RG_vtfp.sct).
 * The linker places .vtfp at the start of RW_IRAM1 (typically 0x200002cc
 * because of system_gd32f4xx.o's .data; the exact address is read from
 * the .map file and passed to PC via this firmware and the vtfp.yaml).
 * vtfp_init() below takes &s_vtfp_header (linker-assigned address) and
 * passes it to the runtime. PC SDK must use the same address (set via
 * the .mklink/project_info.json or vtfp.yaml `memory.header_base`). */
static volatile vtfp_header_t s_vtfp_header __attribute__((section(".vtfp"), aligned(4)));
static uint8_t               s_vtfp_data[5108] __attribute__((section(".vtfp"), aligned(4)));

static void vTaskVTFP(void *pvParameters)
{
    (void)pvParameters;
    vtfp_config_t cfg = {
        .header_base    = (uint32_t)(uintptr_t)&s_vtfp_header,
        .data_addr      = (uint32_t)(uintptr_t)s_vtfp_data,
        .data_size      = sizeof(s_vtfp_data),
        .poll_period_ms = 20u,
        .arm_timeout_ms = 2000u,
        .safety_key     = 0xAA55AA55u,
        .auto_disarm    = true,
    };
    if (vtfp_init(&cfg) != 0) {
        SEGGER_RTT_WriteString(0, "[VTFP] init FAIL\r\n");
        vTaskDelete(NULL);
        return;
    }
    vtfp_user_handlers_init();
    SEGGER_RTT_printf(0, "[VTFP] up: hdr@0x%08x data@0x%08x+%u tick=%ums arm=%ums\r\n",
                      cfg.header_base, cfg.data_addr, cfg.data_size,
                      cfg.poll_period_ms, cfg.arm_timeout_ms);
    for (;;) {
        vtfp_poll();
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
#endif /* USE_VTFP */

/* delay functions using DWT cycle counter (no interrupts, won't corrupt SPI) */
extern uint32_t SystemCoreClock;

static uint8_t _dwt_ready = 0;

/*******************************************************************************
* Description    : 初始化DWT cycle counter用于精确延时
* Input          : none
* Output         : none
* Return         : none
* Application note: 使用CoreDebug寄存器使能DWT跟踪
*******************************************************************************/
static void delay_init(void)
{
    if (_dwt_ready) return;
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    _dwt_ready = 1;
}

/*******************************************************************************
* Description    : 毫秒级延时函数
* Input          : ms - 延时毫秒数
* Output         : none
* Return         : none
* Application note: 使用DWT cycle counter，不受中断影响
*******************************************************************************/
void delay_ms(uint32_t ms)
{
    delay_init();
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = ms * (SystemCoreClock / 1000U);
    while ((uint32_t)(DWT->CYCCNT - start) < ticks) {
        __NOP();
    }
}

/*******************************************************************************
* Description    : LCD GPIO引脚初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 初始化LCD控制引脚：CS/DC/RST/BL及SPI引脚
*******************************************************************************/
void lcd_gpio_init(void)
{
    /* enable GPIOA and GPIOC clocks */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* PA5 - SPI0_SCK (AF5 push-pull) */
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_5);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

    /* PA7 - SPI0_MOSI (AF5 push-pull) */
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_7);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_7);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

    /* PA4 - LCD_CS (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    /* PA3 - LCD_BL (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

    /* PA2 - 备用控制引脚 (GPIO output, push-pull) */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

    /* PC4 - LCD_DC (GPIO output, push-pull) */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);

    /* PC5 - LCD_RST (GPIO output, push-pull) */
    gpio_mode_set(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);

    /* set initial states */
    LCD_CS_HIGH();     /* CS inactive */
    LCD_DC_LOW();      /* command mode */
    LCD_RST_HIGH();    /* release reset */
    LCD_BL_OFF();      /* backlight off */
    gpio_bit_set(GPIOA, GPIO_PIN_2);  /* PA2 low */
}

/*******************************************************************************
* Description    : LCD SPI初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 配置SPI0为主机模式，8位数据，MSB优先
*******************************************************************************/
void lcd_spi_init(void)
{
    spi_parameter_struct spi_param;

    /* enable SPI0 clock */
    rcu_periph_clock_enable(RCU_SPI0);

    /* initialize SPI parameters with default values */
    spi_struct_para_init(&spi_param);

    /* configure SPI0 as master */
    spi_param.device_mode          = SPI_MASTER;
    spi_param.trans_mode           = SPI_TRANSMODE_BDTRANSMIT;
    spi_param.frame_size           = SPI_FRAMESIZE_8BIT;
    spi_param.nss                  = SPI_NSS_SOFT;
    spi_param.endian               = SPI_ENDIAN_MSB;
    spi_param.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;  /* CPOL=0, CPHA=0 */
    spi_param.prescale             = SPI_PSC_4;                /* APB2/4 */

    spi_init(SPI0, &spi_param);

    /* enable SPI0 */
    spi_enable(SPI0);
}

/* Task handles */
static TaskHandle_t lcdTaskHandle = NULL;
static TaskHandle_t systemTaskHandle = NULL;
static TaskHandle_t keyTaskHandle = NULL;

/* Task function prototypes */
static void LCD_Task(void *pvParameters);
static void System_Task(void *pvParameters);
static void Key_Task(void *pvParameters);

/* ========== Menu Definitions ========== */
static void menu_backlight_on(void);
static void menu_backlight_off(void);
static void menu_about(void);
static void menu_mode_auto(void);
static void menu_mode_dots(void);
static void menu_mode_lines(void);
static void menu_mode_text(void);
static void menu_mode_ui(void);
static void menu_mode_numeric(void);
static void menu_mode_colorfill(void);

/* Submenu: Display Mode */
static menu_item_t menu_display[] = {
    {"AutoLoop", .callback = menu_mode_auto},
    {"Dots",     .callback = menu_mode_dots},
    {"Lines",    .callback = menu_mode_lines},
    {"Text",     .callback = menu_mode_text},
    {"UI",       .callback = menu_mode_ui},
    {"Numeric",  .callback = menu_mode_numeric},
    {"Color",    .callback = menu_mode_colorfill},
};

/* Submenu: Settings */
static menu_item_t menu_settings[] = {
    {"BL_ON",   .callback = menu_backlight_on},
    {"BL_OFF",  .callback = menu_backlight_off},
    {"Display", .child = menu_display},
    {"About",   .callback = menu_about},
};

/* Submenu: Info */
static menu_item_t menu_info[] = {
    {"Version", .callback = menu_about},
};

/* Root Menu */
static menu_item_t menu_root[] = {
    {"Settings", .child = &menu_settings[0]},
    {"Info",     .child = &menu_info[0]},
};

/* Link functions - called after all menu arrays are defined */
static void link_display_menu(void) {
    menu_display[0].sibling = &menu_display[1];
    menu_display[1].sibling = &menu_display[2];
    menu_display[2].sibling = &menu_display[3];
    menu_display[3].sibling = &menu_display[4];
    menu_display[4].sibling = &menu_display[5];
    menu_display[5].sibling = &menu_display[6];
    menu_display[6].sibling = NULL;
    for (int i = 0; i < 7; i++) {
        menu_display[i].parent = &menu_settings[2];
    }
}

static void link_settings_menu(void) {
    menu_settings[0].sibling = &menu_settings[1];
    menu_settings[1].sibling = &menu_settings[2];
    menu_settings[2].sibling = &menu_settings[3];
    menu_settings[3].sibling = NULL;
    for (int i = 0; i < 4; i++) {
        menu_settings[i].parent = &menu_root[0];
    }
}

static void link_info_menu(void) {
    menu_info[0].sibling = NULL;
    menu_info[0].parent = &menu_root[1];
}

static void link_root_menu(void) {
    menu_root[0].sibling = &menu_root[1];
    menu_root[1].sibling = NULL;
    menu_root[0].parent = NULL;
    menu_root[1].parent = NULL;
}

/* 菜单回调实现 */
static void menu_backlight_on(void)
{
    LCD_BL_ON();
    SEGGER_RTT_WriteString(0, "[MENU] 背光打开\r\n");
}

static void menu_backlight_off(void)
{
    LCD_BL_OFF();
    SEGGER_RTT_WriteString(0, "[MENU] 背光关闭\r\n");
}

static void menu_about(void)
{
    SEGGER_RTT_WriteString(0, "[MENU] 关于系统 v1.0\r\n");
}

/* 显示模式切换回调 */
static void menu_mode_auto(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_AUTO);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到自动循环模式\r\n");
}

static void menu_mode_dots(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_DOTS);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到点阵模式\r\n");
}

static void menu_mode_lines(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_LINES);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到线条模式\r\n");
}

static void menu_mode_text(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_TEXT);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到文字模式\r\n");
}

static void menu_mode_ui(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_UI);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到UI组件模式\r\n");
}

static void menu_mode_numeric(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_NUMERIC);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到数字显示模式\r\n");
}

static void menu_mode_colorfill(void)
{
    LCD_SetDisplayMode(DISPLAY_MODE_COLORFILL);
    SEGGER_RTT_WriteString(0, "[MENU] 切换到颜色填充模式\r\n");
}
/* ========== 示例菜单定义结束 ========== */

/* Task stack size definitions (in words, not bytes) */
#define LCD_TASK_STACK_SIZE      256
#define SYSTEM_TASK_STACK_SIZE   128
#define KEY_TASK_STACK_SIZE      128

/*******************************************************************************
* Description    : LCD任务
* Input          : pvParameters - 任务参数
* Output         : none
* Return         : none
* Application note: 循环执行LCD测试程序
*******************************************************************************/
static void LCD_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* LCD update logic - call existing LCD test function */
        LCD_Test_Loop();

        /* Delay for 100ms before next update */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*******************************************************************************
* Description    : 系统监控任务
* Input          : pvParameters - 任务参数
* Output         : none
* Return         : none
* Application note: 定期输出系统运行状态日志
*******************************************************************************/
static void System_Task(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* System monitoring - toggle LED or log status */
#ifdef USE_RTT
        SEGGER_RTT_printf(0, "System running...\r\n");
#endif
        /* Delay for 1 second */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*******************************************************************************
* Description    : 按键扫描任务
* Input          : pvParameters - 任务参数
* Output         : none
* Return         : none
* Application note: 初始化菜单系统，每10ms扫描按键并处理菜单导航
*******************************************************************************/
static void Key_Task(void *pvParameters)
{
    (void)pvParameters;

    /* 初始化菜单链接 */
    link_display_menu();
    link_settings_menu();
    link_info_menu();
    link_root_menu();

    /* 初始化菜单系统 */
    menu_init(menu_root);

    for (;;) {
        /* 调试：每500ms打印按键状态 */
        key_debug_print();

        key_event_t event = key_get_event();
        if (event != KEY_EVENT_NONE) {
            menu_set_last_key(event);  /* 物理按键也记一笔,供 VTFP QUERY_STATE 读取 */
            menu_navigate(event);
        }
        /* 10ms扫描周期 */
        vTaskDelay(pdMS_TO_TICKS(KEY_SCAN_PERIOD_MS));
    }
}

/*******************************************************************************
* Description    : 主函数
* Input          : none
* Output         : none
* Return         : none
* Application note: 初始化外设，创建任务，启动调度器
*******************************************************************************/
int main(void)
{
    BaseType_t xReturned;

    /* initialize LCD GPIO and SPI */
    lcd_gpio_init();
    lcd_spi_init();

    /* 初始化按键 */
    key_init();

    /* 初始化 SEGGER RTT */
    SEGGER_RTT_Init();

    /* hardware reset sequence for LCD */
    LCD_RST_LOW();
    for (volatile uint32_t i = 0; i < 10000; i++);
    LCD_RST_HIGH();
    for (volatile uint32_t i = 0; i < 10000; i++);

    /* initialize LCD */
    ST7586_initialize();

    /* Create LCD Task - use high priority to ensure it runs */
    xReturned = xTaskCreate(
        LCD_Task,
        "LCD",
        LCD_TASK_STACK_SIZE,
        NULL,
        3,  /* higher priority */
        &lcdTaskHandle
    );

    if (xReturned != pdPASS) {
        for (;;) { }
    }

    /* Create Key Task */
    xReturned = xTaskCreate(
        Key_Task,
        "Key",
        KEY_TASK_STACK_SIZE,
        NULL,
        2,
        &keyTaskHandle
    );

    if (xReturned != pdPASS) {
        for (;;) { }
    }

    /* Create System Task */
    xReturned = xTaskCreate(
        System_Task,
        "System",
        SYSTEM_TASK_STACK_SIZE,
        NULL,
        1,
        &systemTaskHandle
    );

    if (xReturned != pdPASS) {
        for (;;) { }
    }

#ifdef USE_VTFP
    /* Create VTFP Task (priority = Key, period 20ms) */
    xReturned = xTaskCreate(
        vTaskVTFP,
        "VTFP",
        256,           /* stack (words) */
        NULL,
        2,             /* same priority as Key_Task */
        NULL
    );
    if (xReturned != pdPASS) {
        SEGGER_RTT_WriteString(0, "[VTFP] task create FAIL\r\n");
        for (;;) { }
    }
#endif /* USE_VTFP */

    /* Start scheduler */
    vTaskStartScheduler();

    for (;;) { }
}

/*******************************************************************************
* Description    : 系统Tick钩子函数
* Input          : none
* Output         : none
* Return         : none
* Application note: 每隔Tick中断调用一次
*******************************************************************************/
void vApplicationTickHook(void)
{
}

/*******************************************************************************
* Description    : 空闲任务钩子函数
* Input          : none
* Output         : none
* Return         : none
* Application note: 空闲任务运行时调用，可用于进入低功耗模式
*******************************************************************************/
void vApplicationIdleHook(void)
{
}

/*******************************************************************************
* Description    : 栈溢出检测钩子函数
* Input          : xTask - 任务句柄
*                  pcTaskName - 任务名称
* Output         : none
* Return         : none
* Application note: 栈溢出时调用
*******************************************************************************/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
}
