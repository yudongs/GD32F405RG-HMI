#include "key.h"

/* 按键引脚状态寄存器 */
static uint16_t key_pins[] = {
    KEY_UP_PIN,
    KEY_DOWN_PIN,
    KEY_ENTER_PIN,
    KEY_ESC_PIN
};

/* 消抖计数器 */
static uint8_t key_debounce_cnt[4] = {0};

/* 按键稳定状态 */
static uint8_t key_state[4] = {0};

/* 上次按下的键（用于检测按下沿） */
static uint8_t key_last_press[4] = {0};

/* 长按检测计数器 */
static uint16_t key_longpress_cnt[4] = {0};
#define LONGPRESS_THRESHOLD   100     /* 100 * 10ms = 1秒 */

/*******************************************************************************
* Description    : 按键GPIO初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 配置PB6-PB9为输入上拉模式
*******************************************************************************/
void key_init(void)
{
    /* 使能GPIOC时钟 */
    rcu_periph_clock_enable(KEY_PORT_CLK);

    /* 配置GPIO为输入上拉模式 */
    gpio_mode_set(KEY_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                  KEY_UP_PIN | KEY_DOWN_PIN | KEY_ENTER_PIN | KEY_ESC_PIN);
}

/*******************************************************************************
* Description    : 读取指定按键的原始电平
* Input          : index - 按键索引 0-3
* Output         : none
* Return         : 0=高电平(未按下), 1=低电平(按下)
* Application note: 内部函数
*******************************************************************************/
static uint8_t key_read_pin(uint8_t index)
{
    return gpio_input_bit_get(KEY_PORT, key_pins[index]) == RESET ? 1 : 0;
}

/*******************************************************************************
* Description    : 内部：处理单个按键的扫描和消抖
* Input          : index - 按键索引 0-3
* Output         : none
* Return         : 按键事件类型
* Application note: 消抖3次采样，长按1秒触发
*******************************************************************************/
static key_event_t key_process_single(uint8_t index)
{
    key_event_t event = KEY_EVENT_NONE;
    uint8_t raw;
    uint8_t i = index;

    raw = key_read_pin(i);

    /* 消抖处理 */
    if (raw != key_state[i]) {
        key_debounce_cnt[i]++;
        if (key_debounce_cnt[i] >= KEY_DEBOUNCE_COUNT) {
            key_state[i] = raw;
            key_debounce_cnt[i] = 0;
        }
    } else {
        key_debounce_cnt[i] = 0;
    }

    /* 检测按下沿 */
    if (key_state[i] == 1 && key_last_press[i] == 0) {
        key_last_press[i] = 1;
        key_longpress_cnt[i] = 0;

        /* 立即返回短按事件 */
        switch (index) {
            case 0: event = KEY_EVENT_UP;     break;
            case 1: event = KEY_EVENT_DOWN;   break;
            case 2: event = KEY_EVENT_ENTER;  break;
            case 3: event = KEY_EVENT_ESC;    break;
        }
    }
    /* 检测释放沿 */
    else if (key_state[i] == 0 && key_last_press[i] == 1) {
        key_last_press[i] = 0;
        key_longpress_cnt[i] = 0;
    }
    /* 检测长按 - 在按住期间每LONGPRESS_THRESHOLD产生一次长按事件 */
    else if (key_state[i] == 1 && key_last_press[i] == 1) {
        key_longpress_cnt[i]++;
        if (key_longpress_cnt[i] >= LONGPRESS_THRESHOLD) {
            key_longpress_cnt[i] = 0;
            switch (index) {
                case 0: event = KEY_EVENT_UP_LONG;     break;
                case 1: event = KEY_EVENT_DOWN_LONG;   break;
                case 2: event = KEY_EVENT_ENTER_LONG;   break;
                case 3: event = KEY_EVENT_ESC_LONG;    break;
            }
        }
    }

    return event;
}

/*******************************************************************************
* Description    : 扫描所有按键
* Input          : none
* Output         : none
* Return         : 按键事件（如果同时多个按键按下，优先返回索引小的）
* Application note: 在FreeRTOS任务中每10ms调用一次
*******************************************************************************/
key_event_t key_scan(void)
{
    key_event_t event;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        event = key_process_single(i);
        if (event != KEY_EVENT_NONE) {
            return event;
        }
    }

    return KEY_EVENT_NONE;
}

/*******************************************************************************
* Description    : 获取按键事件（轮询模式，非阻塞）
* Input          : none
* Output         : none
* Return         : 按键事件类型
* Application note: 在FreeRTOS任务中调用此函数获取事件
*******************************************************************************/
key_event_t key_get_event(void)
{
    return key_scan();
}

/*******************************************************************************
* Description    : 调试：打印所有按键引脚状态
* Input          : none
* Output         : none
* Return         : none
* Application note: 用于诊断按键无反应问题
*******************************************************************************/
void key_debug_print(void)
{
    extern void SEGGER_RTT_WriteString(int ChannelIndex, const char* s);
    uint8_t i;

    for (i = 0; i < 4; i++) {
        uint8_t state = key_read_pin(i);
        if (i == 0) {
            if (state) {
                SEGGER_RTT_WriteString(0, "[KEY] UP=1 (pressed)\r\n");
            } else {
                SEGGER_RTT_WriteString(0, "[KEY] UP=0 (released)\r\n");
            }
        } else if (i == 1) {
            if (state) {
                SEGGER_RTT_WriteString(0, "[KEY] DOWN=1 (pressed)\r\n");
            } else {
                SEGGER_RTT_WriteString(0, "[KEY] DOWN=0 (released)\r\n");
            }
        } else if (i == 2) {
            if (state) {
                SEGGER_RTT_WriteString(0, "[KEY] ENTER=1 (pressed)\r\n");
            } else {
                SEGGER_RTT_WriteString(0, "[KEY] ENTER=0 (released)\r\n");
            }
        } else if (i == 3) {
            if (state) {
                SEGGER_RTT_WriteString(0, "[KEY] ESC=1 (pressed)\r\n");
            } else {
                SEGGER_RTT_WriteString(0, "[KEY] ESC=0 (released)\r\n");
            }
        }
    }
}
