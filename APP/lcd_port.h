/*******************************************************************************
* Description    : LCD端口驱动头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: 定义SPI底层接口及数据类型别名
*******************************************************************************/
#ifndef __LCD_PORT_H
#define __LCD_PORT_H

#include "main.h"
#include <stdint.h>

/* type aliases */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/*******************************************************************************
* Description    : SPI发送一个字节
* Input          : dat - 要发送的数据
* Output         : none
* Return         : none
* Application note: 底层SPI传输函数
*******************************************************************************/
void spi_send_byte(u8 dat);

#endif /* __LCD_PORT_H */
