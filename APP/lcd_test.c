#include "lcd_st7586.h"
#include "lcd_test.h"

extern void delay_ms(uint32_t ms);

static void LCD_Test_ColorFill(void)
{
    FillBlack(0x03, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* red   */
    delay_ms(1000);
    FillBlack(0x1C, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* green */
    delay_ms(1000);
    FillBlack(0xE0, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* blue  */
    delay_ms(1000);
    FillBlack(0xFF, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* white */
    delay_ms(1000);
    FillBlack(0x00, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* black */
    delay_ms(1000);
}

static void LCD_Test_Dots(void)
{
    ClrLcdram();
    for (uint16_t x = 10; x < LCD_WIDTH; x += 16) {
        for (uint16_t y = 10; y < LCD_HEIGHT; y += 16) {
            Draw_Dot(x, y);
        }
    }
    delay_ms(1500);
}

static void LCD_Test_Lines(void)
{
    ClrLcdram();
    DrawLine(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    DrawLine(LCD_WIDTH - 1, 0, 0, LCD_HEIGHT - 1);
    DrawLine2(LCD_WIDTH / 2, 0, LCD_WIDTH / 2, LCD_HEIGHT - 1);
    delay_ms(1500);
}

static void LCD_Test_Text(void)
{
    ClrLcdram();
    PrintASCII(10, 10, (const u8 *)"ABCDEFGHIJKLMNOP");
    PrintASCII(10, 26, (const u8 *)"0123456789+-*/=");
    PrintASCII(10, 42, (const u8 *)"abcdefghijklmnop");
    delay_ms(1500);
}

static void LCD_Test_UI(void)
{
    ClrLcdram();
    Box();
    delay_ms(600);
    ClrLcdram();
    Box2();
    delay_ms(600);
    ClrLcdram();
    DynamicBox();
    delay_ms(800);
}

static void LCD_Test_Numeric(void)
{
    ClrLcdram();
    DisplayDecNum(10, 10, 12345, 0, 0, 0, 0);
    DisplayHexValue(10, 30, 0xDEAD, 0);
    DisplayDecNum(10, 50, 0, 0, 0, 0, 0);
    delay_ms(1500);
}

void LCD_Test_Init(void)
{
    ST7586_initialize();
}

void LCD_Test_Loop(void)
{
    while (1) {
        //LCD_Test_ColorFill();
        LCD_Test_Dots();
        LCD_Test_Lines();
        LCD_Test_Text();
        LCD_Test_UI();
        LCD_Test_Numeric();
    }
}
