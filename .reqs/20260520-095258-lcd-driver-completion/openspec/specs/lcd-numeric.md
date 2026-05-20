# 数字显示函数规格

## DisplayDecNum
```
void DisplayDecNum(u8 x, u8 y, u16 num, u8 DecPoint, u8 SignFlag, u8 ReverseDisp, u8 Unit)
```
- 功能: 多功能十进制数字显示
- 参数:
  - x, y: 起始坐标
  - num: 数值
  - DecPoint: 小数位数 (0-4)
  - SignFlag: 符号标志
  - ReverseDisp: 反显标志
  - Unit: 单位类型 (1=% 2=K 3=Hz 4=V 5=A 6=kW 7=C 8=kWh 9=s)
- 特点: 支持清屏、负号、小数点、单位后缀

## ShowDecValue
```
uint8_t ShowDecValue(u8 x, u8 y, u16 HexValue)
```
- 功能: BCD码转十进制显示
- 返回: 转换后的十进制位数

## Show2Num / Show4Num / Show3Num / Show9um
固定位数数字显示，用于:
- Show2Num: 月/日/时/分/秒
- Show4Num: 年份 (2000-2099)
- Show3Num: 三位数
- Show9um: 9位数字 (功率显示)

## DisplayHexValue
```
void DisplayHexValue(u8 x, u8 y, u16 num, u8 dot)
```
- 功能: 十六进制值显示
- 格式: "0x"前缀，支持小数点位置

## DisplayDecNum_9 / DisplayDecNum_Month_9
- DisplayDecNum_9: 9位整数显示
- DisplayDecNum_Month_9: 月发电量显示 (带单位kWh)
