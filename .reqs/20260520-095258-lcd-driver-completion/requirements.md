# LCD驱动移植需求

## 功能需求

### 文字显示类 (5函数)

| ID | 函数名 | 描述 | 优先级 | 依赖 |
|----|--------|------|--------|------|
| REQ-TXT-001 | `PrintASCII` | 6x8点阵ASCII字符显示 | must | ASCIITAB6_8[] |
| REQ-TXT-002 | `InverseASCII68` | 6x8点阵反显(正显/反显控制) | must | ASCIITAB6_8[] |
| REQ-TXT-003 | `PrintASCII812` | 8x12点阵ASCII显示 | should | ASCIITAB8_12[] |
| REQ-TXT-004 | `PrintASCII1216` | 8x16点阵ASCII显示 | should | ASCIITAB816[] |
| REQ-TXT-005 | `DisplayEnDis` | 使能参数反显封装 | should | InverseASCII68 |

### 数字显示类 (9函数)

| ID | 函数名 | 描述 | 优先级 | 依赖 |
|----|--------|------|--------|------|
| REQ-NUM-001 | `DisplayDecNum` | 多功能数字显示(小数点/符号/单位) | must | InverseASCII68 |
| REQ-NUM-002 | `ShowDecValue` | BCD码转十进制显示 | must | InverseASCII68 |
| REQ-NUM-003 | `Show2Num` | 2位数字显示 | must | InverseASCII68 |
| REQ-NUM-004 | `Show4Num` | 4位数字显示 | must | InverseASCII68 |
| REQ-NUM-005 | `Show3Num` | 3位数字显示 | must | InverseASCII68 |
| REQ-NUM-006 | `Show9um` | 9位数字显示 | must | InverseASCII68 |
| REQ-NUM-007 | `DisplayHexValue` | 十六进制值显示 | should | PrintASCII |
| REQ-NUM-008 | `DisplayDecNum_9` | 9位数字专用显示 | should | InverseASCII68 |
| REQ-NUM-009 | `DisplayDecNum_Month_9` | 月发电量显示 | should | InverseASCII68 |

### RTC集成类 (1函数)

| ID | 函数名 | 描述 | 优先级 | 依赖 |
|----|--------|------|--------|------|
| REQ-RTC-001 | `RtcDisplay` | RTC日期时间显示 | could | RTC驱动, Show4Num, Show2Num |

### 其他UI类 (3函数)

| ID | 函数名 | 描述 | 优先级 | 依赖 |
|----|--------|------|--------|------|
| REQ-UI-001 | `PasswordNum` | 密码输入数字显示 | could | InverseASCII68 |
| REQ-UI-002 | `Display_EnableOrDis` | 使能状态显示 | could | InverseASCII68 |
| REQ-UI-003 | `Display_RefluxEnableOrDis` | 回流使能显示 | could | InverseASCII68 |

## 外部依赖

### 已有
- `ChangeTab[]` - 3像素/字节转换表 (APP/lcd_st7586.c)
- `LCD_RS_LOW/HIGH()`, `LCD_CS_LOW/HIGH()` - 宏定义
- `spi_send_byte()` - SPI发送函数

### 需添加
- `ASCIITAB6_8[]` - 6x8字模表 (96字符, 8字节/字符 = 768字节)
- `ASCIITAB8_12[]` - 8x12字模表
- `ASCIITAB816[]` - 8x16字模表

### 需确认
- RTC驱动类型和数据结构 (`RTC_TimeTypeDef`, `RTC_DateTypeDef`)

## 约束条件
1. 显示分辨率: 240x160像素
2. SPI接口: SPI0, APB2/4分频, CPOL=0, CPHA=0
3. 像素格式: 3像素/字节 (ST7586特有)
4. 字模宽度按3字节对齐
