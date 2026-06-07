/*******************************************************************************
* Description    : 按键驱动头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: PB6-PB9四键输入，支持短按/长按/消抖检测
*******************************************************************************/
#ifndef __KEY_H
#define __KEY_H

#include "gd32f4xx.h"

/* 按键引脚定义 - GPIO输入模式 */
#define KEY_PORT        GPIOB
#define KEY_PORT_CLK    RCU_GPIOB

#define KEY_UP_PIN      GPIO_PIN_8    /* PB8 - UP   */
#define KEY_DOWN_PIN    GPIO_PIN_7    /* PB7 - DOWN */
#define KEY_ENTER_PIN   GPIO_PIN_6    /* PB6 - ENTER */
#define KEY_ESC_PIN     GPIO_PIN_9    /* PB9 - ESC  */

/* 按键事件类型 */
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_UP,        /* 向上 */
    KEY_EVENT_DOWN,      /* 向下 */
    KEY_EVENT_ENTER,     /* 确认 */
    KEY_EVENT_ESC,       /* 返回/取消 */
    KEY_EVENT_UP_LONG,   /* 向上-长按 */
    KEY_EVENT_DOWN_LONG,  /* 向下-长按 */
    KEY_EVENT_ENTER_LONG,/* 确认-长按 */
    KEY_EVENT_ESC_LONG   /* 返回-长按 */
} key_event_t;

/* 按键参数配置 */
#define KEY_SCAN_PERIOD_MS    10      /* 扫描周期 10ms */
#define KEY_DEBOUNCE_COUNT    3       /* 消抖采样次数 */

/*******************************************************************************
* Description    : 按键GPIO初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 配置PB6-PB9为输入上拉模式
*******************************************************************************/
void key_init(void);

/*******************************************************************************
* Description    : 扫描所有按键
* Input          : none
* Output         : none
* Return         : 按键事件
* Application note: 每10ms调用一次
*******************************************************************************/
key_event_t key_scan(void);

/*******************************************************************************
* Description    : 获取按键事件
* Input          : none
* Output         : none
* Return         : 按键事件类型
* Application note: 轮询模式，非阻塞
*******************************************************************************/
key_event_t key_get_event(void);

/*******************************************************************************
* Description    : 调试：打印所有按键引脚状态
* Input          : none
* Output         : none
* Return         : none
* Application note: 用于诊断按键无反应问题
*******************************************************************************/
void key_debug_print(void);

#endif /* __KEY_H */
