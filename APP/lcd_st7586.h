/*******************************************************************************
* Description    : ST7586 LCD驱动头文件
* Input          : none
* Output         : none
* Return         : none
* Application note: 240x160单色LCD驱动，支持图形绘制和文字显示
*******************************************************************************/
#ifndef __LCD_ST7586_H
#define __LCD_ST7586_H

#include "main.h"
#include <stdint.h>

/* type aliases */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* display resolution */
#define LCD_WIDTH  240
#define LCD_HEIGHT 160

/* ST7586 commands */
#define CMD_DISPLAY_OFF   0x28
#define CMD_DISPLAY_ON    0x29
#define CMD_COLUMN_ADDR   0x2A
#define CMD_ROW_ADDR      0x2B
#define CMD_WRITE_RAM     0x2C
#define CMD_READ_RAM      0x2E
#define CMD_PARTIAL_AREA  0x30

/* contrast default */
#define LCD_DEFAULT_CONTRAST  0x0C

/*--  C:\Documents and Settings\wangkai\??\liukai.bmp  --*/
/*--  宽度x高度=240*69  LOGO:SOLECTRIA SOLAR --*/
extern const u8 SolectriaSolar[];

/* data format conversion table: 3 pixels per byte */
extern const u8 ChangeTab[];

/* === Basic Functions === */
/*******************************************************************************
* Description    : LCD发送命令
* Input          : cmd - 命令字节
* Output         : none
* Return         : none
* Application note: 通过SPI发送命令到LCD控制器
*******************************************************************************/
void wr_cmd(u8 cmd);

/*******************************************************************************
* Description    : LCD发送数据
* Input          : dat - 数据字节
* Output         : none
* Return         : none
* Application note: 通过SPI发送数据到LCD控制器
*******************************************************************************/
void wr_dat(u8 dat);

/*******************************************************************************
* Description    : 开始连续写入数据
* Input          : none
* Output         : none
* Return         : none
* Application note: 启动GRAM连续写入模式
*******************************************************************************/
void wr_start(void);

/*******************************************************************************
* Description    : 结束连续写入
* Input          : none
* Output         : none
* Return         : none
* Application note: 结束GRAM写入，关闭写信号
*******************************************************************************/
void wr_end(void);

/*******************************************************************************
* Description    : 清除LCD显存
* Input          : none
* Output         : none
* Return         : none
* Application note: 将整个framebuffer清零
*******************************************************************************/
void ClrLcdram(void);

/*******************************************************************************
* Description    : 初始化ST7586 LCD
* Input          : none
* Output         : none
* Return         : none
* Application note: 执行LCD初始化序列
*******************************************************************************/
void ST7586_initialize(void);

/*******************************************************************************
* Description    : LCD开关控制
* Input          : onoff - 0关闭，非零打开
* Output         : none
* Return         : none
* Application note: 控制LCD显示开关
*******************************************************************************/
void LcdOnOff(u8 onoff);

/*******************************************************************************
* Description    : 清除指定区域
* Input          : x1,x2 - X坐标范围
*                  y1,y2 - Y坐标范围
* Output         : none
* Return         : none
* Application note: 填充指定矩形区域为黑色
*******************************************************************************/
void ClrSpecifiedArea(u8 x1, u8 x2, u8 y1, u8 y2);

/*******************************************************************************
* Description    : LCD对比度调暗
* Input          : none
* Output         : none
* Return         : none
* Application note: 降低LCD显示对比度
*******************************************************************************/
void LCD_Darker(void);

/*******************************************************************************
* Description    : LCD对比度调亮
* Input          : none
* Output         : none
* Return         : none
* Application note: 提高LCD显示对比度
*******************************************************************************/
void LCD_Lighter(void);

/* === Framebuffer Functions === */
/*******************************************************************************
* Description    : 清除帧缓冲
* Input          : none
* Output         : none
* Return         : none
* Application note: 清除内部framebuffer
*******************************************************************************/
void FbClear(void);

/*******************************************************************************
* Description    : 刷新帧缓冲到LCD
* Input          : none
* Output         : none
* Return         : none
* Application note: 将framebuffer内容发送到LCD显示
*******************************************************************************/
void FlushToLCD(void);

/*******************************************************************************
* Description    : 可视化帧缓冲输出
* Input          : none
* Output         : none
* Return         : none
* Application note: 调试用，将framebuffer转为可视格式
*******************************************************************************/
void FbDumpVisual(void);

/*******************************************************************************
* Description    : 十六进制格式输出帧缓冲
* Input          : none
* Output         : none
* Return         : none
* Application note: 调试用，以十六进制形式输出
*******************************************************************************/
void FbDumpHex(void);

/* === Graphics === */
/*******************************************************************************
* Description    : 画点
* Input          : x - X坐标
*                  y - Y坐标
* Output         : none
* Return         : none
* Application note: 在指定位置画一个点
*******************************************************************************/
void Draw_Dot(u32 x, u32 y);

/*******************************************************************************
* Description    : 绘制垂直线
* Input          : x - X坐标
*                  y1 - 起始Y坐标
*                  y2 - 结束Y坐标
* Output         : none
* Return         : none
* Application note: 绘制从y1到y2的垂直线
*******************************************************************************/
void YLine(u8 x, u8 y1, u8 y2);

/*******************************************************************************
* Description    : 清除水平线
* Input          : x1 - 起始X坐标
*                  x2 - 结束X坐标
*                  y - Y坐标
* Output         : none
* Return         : none
* Application note: 将指定水平线区域清除
*******************************************************************************/
void ClearXLine(u8 x1, u8 x2, u8 y);

/*******************************************************************************
* Description    : 绘制水平线
* Input          : x1 - 起始X坐标
*                  x2 - 结束X坐标
*                  y - Y坐标
* Output         : none
* Return         : none
* Application note: 绘制从x1到x2的水平线
*******************************************************************************/
void XLine(u8 x1, u8 x2, u8 y);

/*******************************************************************************
* Description    : 显示BMP图片
* Input          : x - 起始X坐标
*                  y - 起始Y坐标
*                  width - 图片宽度
*                  high - 图片高度
*                  pstr - 图片数据指针
* Output         : none
* Return         : none
* Application note: 显示指定大小的BMP图片
*******************************************************************************/
void ShowBMP(u8 x, u8 y, u8 width, u8 high, const u8 *pstr);

/*******************************************************************************
* Description    : 绘制直线
* Input          : x1,y1 - 起点坐标
*                  x2,y2 - 终点坐标
* Output         : none
* Return         : none
* Application note: Bresenham算法绘制直线
*******************************************************************************/
void DrawLine(u32 x1, u32 y1, u32 x2, u32 y2);

/*******************************************************************************
* Description    : 绘制直线2
* Input          : x1,y1 - 起点坐标
*                  x2,y2 - 终点坐标
* Output         : none
* Return         : none
* Application note: 另一种直线绘制算法
*******************************************************************************/
void DrawLine2(u32 x1, u32 y1, u32 x2, u32 y2);

/*******************************************************************************
* Description    : 填充黑色区域
* Input          : dat - 填充数据
*                  x1,x2 - X坐标范围
*                  y1,y2 - Y坐标范围
* Output         : none
* Return         : none
* Application note: 填充指定矩形区域
*******************************************************************************/
void FillBlack(u8 dat, u8 x1, u8 x2, u8 y1, u8 y2);

/*******************************************************************************
* Description    : 清除黑色区域
* Input          : dat - 清除数据
*                  x1,x2 - X坐标范围
*                  y1,y2 - Y坐标范围
* Output         : none
* Return         : none
* Application note: 清除指定矩形区域
*******************************************************************************/
void ClearBlack(u8 dat, u8 x1, u8 x2, u8 y1, u8 y2);

/* === UI Components === */
/*******************************************************************************
* Description    : 显示方框
* Input          : none
* Output         : none
* Return         : none
* Application note: 绘制一个矩形方框
*******************************************************************************/
void Box(void);

/*******************************************************************************
* Description    : 显示方框2
* Input          : none
* Output         : none
* Return         : none
* Application note: 绘制另一个样式的矩形方框
*******************************************************************************/
void Box2(void);

/*******************************************************************************
* Description    : 显示保持电阻框
* Input          : none
* Output         : none
* Return         : none
* Application note: UI组件
*******************************************************************************/
void HoldingResgBox(void);

/*******************************************************************************
* Description    : 显示电阻启用框
* Input          : none
* Output         : none
* Return         : none
* Application note: UI组件
*******************************************************************************/
void ResgEnableBox(void);

/*******************************************************************************
* Description    : 显示动态方框
* Input          : none
* Output         : none
* Return         : none
* Application note: 带动画效果的方框
*******************************************************************************/
void DynamicBox(void);

/*******************************************************************************
* Description    : 显示70kw坐标轴
* Input          : none
* Output         : none
* Return         : none
* Application note: 绘制70kw功率对应的坐标轴
*******************************************************************************/
void CoordinateAxis70kw(void);

/*******************************************************************************
* Description    : 显示60kw坐标轴
* Input          : none
* Output         : none
* Return         : none
* Application note: 绘制60kw功率对应的坐标轴
*******************************************************************************/
void CoordinateAxis60kw(void);

/*******************************************************************************
* Description    : 显示50kw坐标轴
* Input          : none
* Output         : none
* Return         : none
* Application note: 绘制50kw功率对应的坐标轴
*******************************************************************************/
void CoordinateAxis50kw(void);

/*******************************************************************************
* Description    : 显示设置方框
* Input          : none
* Output         : none
* Return         : none
* Application note: UI组件
*******************************************************************************/
void BoxSetting(void);

/* === Text Display === */
/*******************************************************************************
* Description    : 显示ASCII字符
* Input          : x - X坐标
*                  y - Y坐标
*                  asciicode - ASCII码指针
* Output         : none
* Return         : none
* Application note: 使用8x16字体显示单个字符
*******************************************************************************/
void PrintASCII(u8 x, u8 y, const u8 *asciicode);

/*******************************************************************************
* Description    : 反显ASCII字符68
* Input          : x - X坐标
*                  y - Y坐标
*                  a - 反显控制
*                  asciicode - ASCII码指针
* Output         : none
* Return         : none
* Application note: 特定反显模式显示字符
*******************************************************************************/
void InverseASCII68(u8 x, u8 y, u8 a, const u8 *asciicode);

/*******************************************************************************
* Description    : 显示启用/禁用状态
* Input          : x - X坐标
*                  y - Y坐标
*                  a - 控制参数
*                  asciicode - ASCII码指针
* Output         : none
* Return         : none
* Application note: 根据状态显示不同样式
*******************************************************************************/
void DisplayEnDis(u8 x, u8 y, u8 a, const u8 *asciicode);

/*******************************************************************************
* Description    : 显示ASCII 8x12字符
* Input          : x - X坐标
*                  y - Y坐标
*                  asciicode - ASCII码指针
* Output         : none
* Return         : none
* Application note: 使用8x12字体显示
*******************************************************************************/
void PrintASCII812(u8 x, u8 y, const u8 *asciicode);

/*******************************************************************************
* Description    : 显示ASCII 12x16字符
* Input          : x - X坐标
*                  y - Y坐标
*                  asciicode - ASCII码指针
* Output         : none
* Return         : none
* Application note: 使用12x16字体显示
*******************************************************************************/
void PrintASCII1216(u8 x, u8 y, u8 *asciicode);

/* === Numeric Display === */
/*******************************************************************************
* Description    : 显示十进制数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  DecPoint - 小数点位置
*                  SignFlag - 正负号标志
*                  ReverseDisp - 反显标志
*                  Unit - 单位
* Output         : none
* Return         : none
* Application note: 格式化显示数字
*******************************************************************************/
void DisplayDecNum(u8 x, u8 y, u16 num, u8 DecPoint, u8 SignFlag, u8 ReverseDisp, u8 Unit);

/*******************************************************************************
* Description    : 显示十进制值
* Input          : x - X坐标
*                  y - Y坐标
*                  HexValue - 十六进制值
* Output         : none
* Return         : 成功显示的位数
* Application note: 将十六进制值作为十进制显示
*******************************************************************************/
uint8_t ShowDecValue(u8 x, u8 y, u16 HexValue);

/*******************************************************************************
* Description    : 显示2位数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  ReverseDisp - 反显标志
* Output         : none
* Return         : none
* Application note: 显示2位数字
*******************************************************************************/
void Show2Num(u8 x, u8 y, u8 num, u8 ReverseDisp);

/*******************************************************************************
* Description    : 显示4位数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  ReverseDisp - 反显标志
* Output         : none
* Return         : none
* Application note: 显示4位数字
*******************************************************************************/
void Show4Num(u8 x, u8 y, u16 num, u8 ReverseDisp);

/*******************************************************************************
* Description    : 显示3位数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
* Output         : none
* Return         : none
* Application note: 显示3位数字
*******************************************************************************/
void Show3Num(u8 x, u8 y, u32 num);

/*******************************************************************************
* Description    : 显示9位数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  ReverseDisp - 反显标志
* Output         : none
* Return         : none
* Application note: 显示9位数字
*******************************************************************************/
void Show9um(u8 x, u8 y, u32 num, u8 ReverseDisp);

/*******************************************************************************
* Description    : 显示十六进制值
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  dot - 小数点位置
* Output         : none
* Return         : none
* Application note: 以十六进制格式显示数字
*******************************************************************************/
void DisplayHexValue(u8 x, u8 y, u16 num, u8 dot);

/*******************************************************************************
* Description    : 显示9位十进制数字
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  ReverseDisp - 反显标志
* Output         : none
* Return         : none
* Application note: 显示9位十进制数
*******************************************************************************/
void DisplayDecNum_9(u8 x, u8 y, u32 num, u8 ReverseDisp);

/*******************************************************************************
* Description    : 显示月份9位十进制
* Input          : x - X坐标
*                  y - Y坐标
*                  num - 数字值
*                  ReverseDisp - 反显标志
* Output         : none
* Return         : none
* Application note: 用于月份显示的9位数字
*******************************************************************************/
void DisplayDecNum_Month_9(u8 x, u8 y, u32 num, u8 ReverseDisp);

/* === Password & Status Display === */
/*******************************************************************************
* Description    : 显示密码数字
* Input          : x - X坐标
*                  y - Y坐标
*                  color - 颜色
*                  num - 数字
* Output         : none
* Return         : none
* Application note: 密码输入框数字显示
*******************************************************************************/
void PasswordNum(u8 x, u8 y, u8 color, u8 num);

/*******************************************************************************
* Description    : 显示启用/禁用状态
* Input          : x - X坐标
*                  y - Y坐标
*                  ReverseDisp - 反显标志
*                  EnableRegValue - 启用寄存器值
* Output         : none
* Return         : none
* Application note: 显示Enable/Disable状态
*******************************************************************************/
void Display_EnableOrDis(u8 x, u8 y, u8 ReverseDisp, u8 EnableRegValue);

/*******************************************************************************
* Description    : 显示回流启用/禁用状态
* Input          : x - X坐标
*                  y - Y坐标
*                  ReverseDisp - 反显标志
*                  EnableRegValue - 启用寄存器值
* Output         : none
* Return         : none
* Application note: 显示Reflux Enable/Disable状态
*******************************************************************************/
void Display_RefluxEnableOrDis(u8 x, u8 y, u8 ReverseDisp, u8 EnableRegValue);

/*******************************************************************************
* Description    : 取 LCD framebuffer 裸指针(供 VTFP / lcd_server 远程读屏)
* Input          : none
* Output         : none
* Return         : 指向 4800 字节 framebuffer 的 const 指针
* Application note: 不拷贝,只暴露指针。调用方需保证期间不被 LCD_Task 改写。
*******************************************************************************/
const u8 *lcd_get_fb(void);

/*******************************************************************************
* Description    : 取 framebuffer 字节数
*******************************************************************************/
u32 lcd_get_fb_size(void);

#endif /* __LCD_ST7586_H */
