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
void wr_cmd(u8 cmd);
void wr_dat(u8 dat);
void ClrLcdram(void);
void ST7586_initialize(void);
void LcdOnOff(u8 onoff);
void ClrSpecifiedArea(u8 x1, u8 x2, u8 y1, u8 y2);
void LCD_Darker(void);
void LCD_Lighter(void);

/* === Graphics === */
void Draw_Dot(u32 x, u32 y);
void YLine(u8 x, u8 y1, u8 y2);
void ClearXLine(u8 x1, u8 x2, u8 y);
void XLine(u8 x1, u8 x2, u8 y);
void ShowBMP(u8 x, u8 y, u8 width, u8 high, const u8 *pstr);
void DrawLine(u32 x1, u32 y1, u32 x2, u32 y2);
void DrawLine2(u32 x1, u32 y1, u32 x2, u32 y2);
void FillBlack(u8 dat, u8 x1, u8 x2, u8 y1, u8 y2);
void ClearBlack(u8 dat, u8 x1, u8 x2, u8 y1, u8 y2);

/* === UI Components === */
void Box(void);
void Box2(void);
void HoldingResgBox(void);
void ResgEnableBox(void);
void DynamicBox(void);
void CoordinateAxis70kw(void);
void CoordinateAxis60kw(void);
void CoordinateAxis50kw(void);
void BoxSetting(void);

/* === Text Display === */
void PrintASCII(u8 x, u8 y, const u8 *asciicode);
void InverseASCII68(u8 x, u8 y, u8 a, const u8 *asciicode);
void DisplayEnDis(u8 x, u8 y, u8 a, const u8 *asciicode);
void PrintASCII812(u8 x, u8 y, const u8 *asciicode);
void PrintASCII1216(u8 x, u8 y, u8 *asciicode);

/* === Numeric Display === */
void DisplayDecNum(u8 x, u8 y, u16 num, u8 DecPoint, u8 SignFlag, u8 ReverseDisp, u8 Unit);
uint8_t ShowDecValue(u8 x, u8 y, u16 HexValue);
void Show2Num(u8 x, u8 y, u8 num, u8 ReverseDisp);
void Show4Num(u8 x, u8 y, u16 num, u8 ReverseDisp);
void Show3Num(u8 x, u8 y, u32 num);
void Show9um(u8 x, u8 y, u32 num, u8 ReverseDisp);
void DisplayHexValue(u8 x, u8 y, u16 num, u8 dot);
void DisplayDecNum_9(u8 x, u8 y, u32 num, u8 ReverseDisp);
void DisplayDecNum_Month_9(u8 x, u8 y, u32 num, u8 ReverseDisp);

/* === Password & Status Display === */
void PasswordNum(u8 x, u8 y, u8 color, u8 num);
void Display_EnableOrDis(u8 x, u8 y, u8 ReverseDisp, u8 EnableRegValue);
void Display_RefluxEnableOrDis(u8 x, u8 y, u8 ReverseDisp, u8 EnableRegValue);

#endif /* __LCD_ST7586_H */
