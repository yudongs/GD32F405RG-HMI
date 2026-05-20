# LCD驱动移植任务清单

## Phase 1: 基础文字显示
- [ ] T1: 从参考文件LCD.c复制ASCIITAB6_8字模表到lcd_st7586.c
- [ ] T2: 从参考文件LCD.c复制ASCIITAB8_12字模表到lcd_st7586.c
- [ ] T3: 从参考文件LCD.c复制ASCIITAB816字模表到lcd_st7586.c
- [ ] T4: 实现PrintASCII函数并添加声明
- [ ] T5: 实现InverseASCII68函数并添加声明
- [ ] T6: 实现DisplayEnDis函数并添加声明
- [ ] T7: 编译验证

## Phase 2: 数字显示核心
- [ ] T8: 实现Show2Num函数并添加声明
- [ ] T9: 实现Show4Num函数并添加声明
- [ ] T10: 实现Show3Num函数并添加声明
- [ ] T11: 实现Show9um函数并添加声明
- [ ] T12: 实现ShowDecValue函数并添加声明
- [ ] T13: 实现DisplayDecNum函数并添加声明
- [ ] T14: 编译验证

## Phase 3: 扩展功能
- [ ] T15: 实现PrintASCII812函数并添加声明
- [ ] T16: 实现PrintASCII1216函数并添加声明
- [ ] T17: 实现DisplayHexValue函数并添加声明
- [ ] T18: 实现DisplayDecNum_9函数并添加声明
- [ ] T19: 实现DisplayDecNum_Month_9函数并添加声明
- [ ] T20: 编译验证

## Phase 4: RTC和UI
- [ ] T21: 实现PasswordNum函数并添加声明
- [ ] T22: 实现Display_EnableOrDis函数并添加声明
- [ ] T23: 实现Display_RefluxEnableOrDis函数并添加声明
- [ ] T24: 实现RtcDisplay函数并添加声明 (依赖RTC驱动)
- [ ] T25: 编译验证
- [ ] T26: 硬件测试验证
