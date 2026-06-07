/*******************************************************************************
* Description    : LCD测试程序头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: LCD屏幕诊断测试程序
*******************************************************************************/
#ifndef __LCD_TEST_H
#define __LCD_TEST_H

/* 显示模式枚举 */
typedef enum {
    DISPLAY_MODE_AUTO = 0,  /* 自动循环模式 */
    DISPLAY_MODE_DOTS,      /* 点阵模式 */
    DISPLAY_MODE_LINES,     /* 线条模式 */
    DISPLAY_MODE_TEXT,      /* 文字模式 */
    DISPLAY_MODE_UI,        /* UI组件模式 */
    DISPLAY_MODE_NUMERIC,   /* 数字显示模式 */
    DISPLAY_MODE_COLORFILL, /* 颜色填充模式 */
    DISPLAY_MODE_COUNT      /* 模式总数 */
} display_mode_t;

/*******************************************************************************
* Description    : LCD测试初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 初始化LCD并执行测试
*******************************************************************************/
void LCD_Test_Init(void);

/*******************************************************************************
* Description    : LCD测试循环
* Input          : none
* Output         : none
* Return         : none
* Application note: 循环执行LCD各项测试
*******************************************************************************/
void LCD_Test_Loop(void);

/*******************************************************************************
* Description    : 设置显示模式
* Input          : mode - 显示模式
* Output         : none
* Return         : none
*******************************************************************************/
void LCD_SetDisplayMode(display_mode_t mode);

/*******************************************************************************
* Description    : 获取当前显示模式
* Input          : none
* Output         : none
* Return         : 当前显示模式
*******************************************************************************/
display_mode_t LCD_GetDisplayMode(void);

#endif /* __LCD_TEST_H */
