# 约束条件

## 硬件约束
- MCU: GD32F405RG, 168MHz
- LCD: ST7586驱动IC, LM240160面板, 240x160像素
- SPI: SPI0, APB2/4=42MHz, CPOL=0, CPHA=0

## 显示约束
- 像素格式: 3像素/字节 (ST7586特有)
- 字模宽度: 按3字节对齐
- 字体: 6x8, 8x12, 8x16三种点阵

## 依赖约束
- InverseASCII68 是数字显示的基础
- RtcDisplay 依赖 RTC_TimeTypeDef/RTC_DateTypeDef
