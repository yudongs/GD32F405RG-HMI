# LCD 屏幕诊断测试程序 - 需求包

## 快速索引

| 文件 | 用途 |
|------|------|
| `requirements.md` | 功能需求定义 |
| `behavior-spec.md` | 行为规范与测试场景 |
| `openspec/proposal.md` | 实现提案 |
| `openspec/tasks.md` | 任务分解 |

## 核心需求

- 诊断调试用全量 LCD 测试
- 上电自动运行，持续循环
- 独立模块 `APP/lcd_test.c` + `lcd_test.h`

## 测试项（按序循环）

1. 纯色填充（红/绿/蓝/白/黑）
2. 像素点阵
3. 线条绘制
4. 文字显示
5. UI 组件（Box 系列）
6. 数值显示

## 约束

- 使用现有 `lcd_st7586` 驱动 API
- 每项测试持续 1-2 秒
- 不含按钮交互

## 验证命令

```bash
UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"
```
