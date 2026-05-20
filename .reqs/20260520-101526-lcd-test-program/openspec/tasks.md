# LCD 测试程序 - 任务分解

## 任务清单

| # | 任务 | 依赖 | 优先级 |
|---|------|------|--------|
| 1 | 创建 `APP/lcd_test.h` 头文件 | - | must |
| 2 | 创建 `APP/lcd_test.c` 实现文件 | 1 | must |
| 3 | 实现 `LCD_Test_Init()` | - | must |
| 4 | 实现 `LCD_Test_ColorFill()` | 1 | must |
| 5 | 实现 `LCD_Test_Dots()` | 1 | must |
| 6 | 实现 `LCD_Test_Lines()` | 1 | must |
| 7 | 实现 `LCD_Test_Text()` | 1 | must |
| 8 | 实现 `LCD_Test_UI()` | 1 | must |
| 9 | 实现 `LCD_Test_Numeric()` | 1 | must |
| 10 | 实现 `LCD_Test_Loop()` 主循环 | 3-9 | must |
| 11 | 在 main.c 中调用测试入口 | 10 | must |
| 12 | 编译验证 | 11 | must |

## 修复警告

- `lcd_st7586.c:508` 需声明 `delay_ms` 函数（在 lcd_test.c 中添加 `extern void delay_ms(u32 ms);`）

## 验证

```bash
UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"
```
