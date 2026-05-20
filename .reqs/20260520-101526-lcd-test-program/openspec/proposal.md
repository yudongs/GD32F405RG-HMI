# LCD 测试程序 - 实现提案

## 架构决策

- **测试模块分离**：独立 `APP/lcd_test.c` + `lcd_test.h`
- **循环调度**：主循环中顺序调用各项测试
- **延时管理**：复用现有 `delay_ms` 函数

## 测试序列

```
LCD_Test_Loop()
├── LCD_Test_ColorFill()    // 纯色填充
├── LCD_Test_Dots()         // 像素点阵
├── LCD_Test_Lines()        // 线条绘制
├── LCD_Test_Text()         // 文字显示
├── LCD_Test_UI()           // UI 组件
└── LCD_Test_Numeric()       // 数值显示
```

## 关键 API（来自 lcd_st7586.h）

```c
// 基础
void ST7586_initialize(void);
void ClrLcdram(void);
void ClrSpecifiedArea(u8 x1, u8 x2, u8 y1, u8 y2);

// 填充
void FillBlack(u8 dat, u8 x1, u8 x2, u8 y1, u8 y2);

// 图形
void Draw_Dot(u32 x, u32 y);
void DrawLine(u32 x1, u32 y1, u32 x2, u32 y2);
void DrawLine2(u32 x1, u32 y1, u32 x2, u32 y2);

// 文字
void PrintASCII(u8 x, u8 y, const u8 *asciicode);
void PrintASCII1216(u8 x, u8 y, u8 *asciicode);

// UI
void Box(void);
void Box2(void);
void DynamicBox(void);

// 数值
void DisplayDecNum(u8 x, u8 y, u16 num, u8 DecPoint, u8 SignFlag, u8 ReverseDisp, u8 Unit);
void DisplayHexValue(u8 x, u8 y, u16 num, u8 dot);
```

## 公开接口

```c
// lcd_test.h
void LCD_Test_Loop(void);       // 主测试循环
void LCD_Test_Init(void);        // 初始化（调用 ST7586_initialize）
```
