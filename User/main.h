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

#endif
