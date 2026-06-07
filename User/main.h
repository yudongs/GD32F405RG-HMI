/*******************************************************************************
* Description    : 主头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: 定义LCD控制引脚及宏，包含所有外设驱动头文件
*******************************************************************************/
#ifndef __MAIN_H
#define __MAIN_H

#include "gd32f4xx.h"

/* LCD control pin definitions */
#define LCD_CS_PORT     GPIOA
#define LCD_CS_PIN      GPIO_PIN_4

#define LCD_DC_PORT     GPIOC
#define LCD_DC_PIN      GPIO_PIN_4

#define LCD_RST_PORT    GPIOC
#define LCD_RST_PIN     GPIO_PIN_5

#define LCD_BL_PORT     GPIOA
#define LCD_BL_PIN      GPIO_PIN_3

#define LCD_SPI         SPI0
#define LCD_SPI_PORT    GPIOA
#define LCD_SPI_SCK_PIN GPIO_PIN_5
#define LCD_SPI_MOSI_PIN GPIO_PIN_7
#define LCD_SPI_AF      GPIO_AF_5

/* LCD control macros */
#define LCD_CS_HIGH()   gpio_bit_set(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_CS_LOW()    gpio_bit_reset(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_DC_HIGH()   gpio_bit_set(LCD_DC_PORT, LCD_DC_PIN)
#define LCD_DC_LOW()    gpio_bit_reset(LCD_DC_PORT, LCD_DC_PIN)
#define LCD_RST_HIGH()  gpio_bit_set(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_RST_LOW()   gpio_bit_reset(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_BL_ON()     gpio_bit_set(LCD_BL_PORT, LCD_BL_PIN)
#define LCD_BL_OFF()    gpio_bit_reset(LCD_BL_PORT, LCD_BL_PIN)

void lcd_gpio_init(void);
void lcd_spi_init(void);
void delay_ms(uint32_t ms);

/* VTFP 虚拟测试夹具 — 设为 1 启用,0 关闭(默认 0,生产固件不带)
 * 启用时:
 *   - main.c 创建一个 vTaskVTFP 任务
 *   - Project/Objects/GD32F405RG_vtfp.sct 把 .vtfp section 放 0x20000000
 *   - 链接器: 5KB SRAM 给 VTFP header+data,FreeRTOS heap 减 2KB
 *   - PC 端: 通过 MCP server 远程按键/读屏/切模式
 */
#ifndef USE_VTFP
#define USE_VTFP   0   /* 默认关闭,开发者手动打开 */
#endif

#endif
