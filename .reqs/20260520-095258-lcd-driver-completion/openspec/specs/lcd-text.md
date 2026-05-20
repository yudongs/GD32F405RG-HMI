# 文字显示函数规格

## PrintASCII
```
void PrintASCII(u8 x, u8 y, const u8 *asciicode)
```
- 功能: 在坐标(x,y)显示6x8点阵ASCII字符串
- 参数: x,y坐标, asciicode指向ASCII字符串
- 行为: 遍历字符串，每个字符调用字模转换输出

## InverseASCII68
```
void InverseASCII68(u8 x, u8 y, u8 a, const u8 *asciicode)
```
- 功能: 显示6x8点阵，支持正显/反显
- 参数: a=1正显, a=0反显
- 行为: 输出字节与0xFF异或实现反显

## PrintASCII812
```
void PrintASCII812(u8 x, u8 y, const u8 *asciicode)
```
- 功能: 显示8x12点阵ASCII字符
- 差异: 使用ASCIITAB8_12，字模宽度4字节

## PrintASCII1216
```
void PrintASCII1216(u8 x, u8 y, u8 *asciicode)
```
- 功能: 显示8x16点阵ASCII字符
- 差异: 使用ASCIITAB816，字模宽度4字节

## DisplayEnDis
```
void DisplayEnDis(u8 x, u8 y, u8 a, const u8 *asciicode)
```
- 功能: 7字符宽度使能参数显示
- 行为: 先清7字符位置，再显示目标字符
