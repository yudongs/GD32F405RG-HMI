#include "lcd_st7586.h"
#include "lcd_test.h"

extern void delay_ms(uint32_t ms);

/*******************************************************************************
* Description    : LCD颜色填充测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 依次显示红、绿、蓝、白、黑五种颜色
*******************************************************************************/
static void LCD_Test_ColorFill(void)
{
    FillBlack(0x03, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* red   */
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1000);
    FillBlack(0x1C, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* green */
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1000);
    FillBlack(0xE0, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* blue  */
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1000);
    FillBlack(0xFF, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* white */
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1000);
    FillBlack(0x00, 0, LCD_WIDTH - 1, 0, LCD_HEIGHT - 1);  /* black */
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1000);
}

/*******************************************************************************
* Description    : LCD点阵显示测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 显示网格点阵
*******************************************************************************/
static void LCD_Test_Dots(void)
{
    ClrLcdram();
    for (uint16_t x = 10; x < LCD_WIDTH; x += 16) {
        for (uint16_t y = 10; y < LCD_HEIGHT; y += 16) {
            Draw_Dot(x, y);
        }
    }
    FbDumpVisual();
    FlushToLCD();
    delay_ms(5000);
}

/*******************************************************************************
* Description    : LCD线条绘制测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 显示交叉线条和对角线
*******************************************************************************/
static void LCD_Test_Lines(void)
{
    ClrLcdram();
    DrawLine(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    DrawLine(LCD_WIDTH - 1, 0, 0, LCD_HEIGHT - 1);
    DrawLine2(LCD_WIDTH / 2, 0, LCD_WIDTH / 2, LCD_HEIGHT - 1);
    FbDumpVisual();
    FlushToLCD();
    delay_ms(5000);
}

/*******************************************************************************
* Description    : LCD文字显示测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 显示ASCII字符
*******************************************************************************/
static void LCD_Test_Text(void)
{
    ClrLcdram();
    PrintASCII(10, 10, (const u8 *)"ABCDEFGHIJKLMNOP");
    PrintASCII(10, 26, (const u8 *)"0123456789+-*/=");
    PrintASCII(10, 42, (const u8 *)"abcdefghijklmnop");
    FbDumpVisual();
    FlushToLCD();
    delay_ms(1500);
}

/*******************************************************************************
* Description    : LCD UI组件测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 显示各种UI框和动态效果
*******************************************************************************/
static void LCD_Test_UI(void)
{
    ClrLcdram();
    Box();
    FbDumpVisual();
    FlushToLCD();
    delay_ms(600);
    ClrLcdram();
    Box2();
    FbDumpVisual();
    FlushToLCD();
    delay_ms(600);
    ClrLcdram();
    DynamicBox();
    FbDumpVisual();
    FlushToLCD();
    delay_ms(800);
}

/*******************************************************************************
* Description    : LCD数字显示测试
* Input          : none
* Output         : none
* Return         : none
* Application note: 显示十进制和十六进制数字
*******************************************************************************/
static void LCD_Test_Numeric(void)
{
    ClrLcdram();
    DisplayDecNum(10, 10, 12345, 0, 0, 0, 0);
    DisplayHexValue(10, 30, 0xDEAD, 0);
    DisplayDecNum(10, 50, 0, 0, 0, 0, 0);
    FbDumpVisual();
    FlushToLCD();
    delay_ms(5000);
}

static display_mode_t g_display_mode = DISPLAY_MODE_AUTO;
static uint8_t g_display_auto_index = 0;

/*******************************************************************************
* Description    : LCD测试初始化
* Input          : none
* Output         : none
* Return         : none
* Application note: 初始化LCD显示屏
*******************************************************************************/
void LCD_Test_Init(void)
{
    ST7586_initialize();
}

/*******************************************************************************
* Description    : 设置显示模式
* Input          : mode - 显示模式
* Output         : none
* Return         : none
*******************************************************************************/
void LCD_SetDisplayMode(display_mode_t mode)
{
    g_display_mode = mode;
    g_display_auto_index = 0;
}

/*******************************************************************************
* Description    : 获取当前显示模式
* Input          : none
* Output         : none
* Return         : 当前显示模式
*******************************************************************************/
display_mode_t LCD_GetDisplayMode(void)
{
    return g_display_mode;
}

/*******************************************************************************
* Description    : 执行当前模式的显示
* Input          : none
* Output         : none
* Return         : none
* Application note: 在任务中循环调用
*******************************************************************************/
void LCD_RefreshDisplay(void)
{
    switch (g_display_mode) {
        case DISPLAY_MODE_DOTS:
            LCD_Test_Dots();
            break;
        case DISPLAY_MODE_LINES:
            LCD_Test_Lines();
            break;
        case DISPLAY_MODE_TEXT:
            LCD_Test_Text();
            break;
        case DISPLAY_MODE_UI:
            LCD_Test_UI();
            break;
        case DISPLAY_MODE_NUMERIC:
            LCD_Test_Numeric();
            break;
        case DISPLAY_MODE_COLORFILL:
            LCD_Test_ColorFill();
            break;
        case DISPLAY_MODE_AUTO:
        default:
            /* 自动循环模式 */
            switch (g_display_auto_index) {
                case 0: LCD_Test_Dots();      break;
                case 1: LCD_Test_Lines();     break;
                case 2: LCD_Test_Text();      break;
                case 3: LCD_Test_UI();        break;
                case 4: LCD_Test_Numeric();   break;
                case 5: LCD_Test_ColorFill(); break;
                default: g_display_auto_index = 0; break;
            }
            g_display_auto_index = (g_display_auto_index + 1) % 6;
            break;
    }
}

/*******************************************************************************
* Description    : LCD测试循环（兼容旧接口）
* Input          : none
* Output         : none
* Return         : none
* Application note: 循环执行LCD各项测试
*******************************************************************************/
void LCD_Test_Loop(void)
{
    LCD_RefreshDisplay();
}
