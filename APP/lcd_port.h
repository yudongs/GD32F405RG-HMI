#ifndef __LCD_PORT_H
#define __LCD_PORT_H

#include "main.h"
#include <stdint.h>

/* type aliases */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* SPI byte send declaration */
void spi_send_byte(u8 dat);

#endif /* __LCD_PORT_H */
