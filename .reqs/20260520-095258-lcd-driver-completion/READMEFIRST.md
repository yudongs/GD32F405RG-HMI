# LCD驱动完整移植 - 需求包

## 概述
将参考文件 `c:\Users\yds\Desktop\LCD` 中的18个缺失LCD驱动函数移植到 GD32F405RG 项目。

## 读取顺序
1. `requirements.md` - 功能需求清单
2. `openspec/specs/lcd-text.md` - 文字显示函数规格
3. `openspec/specs/lcd-numeric.md` - 数字显示函数规格
4. `openspec/specs/lcd-fonts.md` - 字模表规格

## 验证命令
```bash
UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"
```

## 关键约束
- 字模表位于 `APP/lcd_st7586.c`
- 文字函数依赖 `ChangeTab[]` (已有)
- RTC显示函数需要 `RTC_TimeTypeDef`, `RTC_DateTypeDef`
- SPI接口: PA5=SCK, PA7=MOSI, PA4=CS, PC4=DC, PC5=RST, PA3=BL

## 开放问题
- [ ] 确认字模表数据可从参考文件直接复制
- [ ] 确认RTC驱动已存在或需同步移植
