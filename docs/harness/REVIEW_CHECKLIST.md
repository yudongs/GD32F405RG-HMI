# Harness 文档一致性审核清单

> 每次提交前逐项检查，确保入口文档与深层文档同步。
> 对应约束：`doc-freshness`。

## 提交前必查

### 1. 入口文档一致性

- [ ] `AGENTS.md` 的 Build system 与 `tooling-plan.md` 一致（应为 `Keil MDK-ARM`）
- [ ] `AGENTS.md` 验证命令与 `testing-and-gates.md` 一致
- [ ] `CLAUDE.md` 与 `AGENTS.md` 规则一致，仅末尾追加 Claude 专属提示

### 2. 术语同步

- [ ] 新增功能/模块涉及新概念 → 检查 `glossary.md` 是否已有定义
- [ ] 若无定义 → 先补充 glossary，再继续实现
- [ ] 禁止在 PR/Issue 中引入未登记的术语

### 3. Grill 协议触发

- [ ] 涉及外设驱动修改（ADC/CAN/USART/SPI/I2C/DMA 等）→ 确认已走 Grill 协议
- [ ] 涉及中断处理逻辑修改 → 确认已走 Grill 协议
- [ ] 涉及引脚复用/板级配置修改 → 确认已走 Grill 协议
- [ ] Grill 输出已落地到 `docs/plan/<feature>.md` 或 Issue

### 4. 模块边界

- [ ] 驱动层（`GD32_StdPeriph_Driver/`）未调用应用层代码
- [ ] 未引入跨模块深 import（绕过 barrel 文件）
- [ ] 新增模块已登记到 `architecture-constraints.md`

### 5. 硬件验证

- [ ] 外设驱动变更后已执行 `python -m mklink flash` 烧录验证
- [ ] 变更涉及运行时行为 → 已执行 `python -m mklink rtt` 观测
- [ ] 异常行为 → 已使用 `mklink hardfault` 分析

### 6. 构建 Gate

- [ ] 代码变更后已运行 Keil 构建：`UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"`
- [ ] 构建无错误/警告

## 审核记录

| 日期 | 提交/PR | 检查人 | 问题 | 处理 |
|------|---------|--------|------|------|
| — | — | — | — | — |

## 升级为自动化的条件

- 仓库引入 CI/CD
- 文档一致性检查脚本验证通过
- 团队确认自动化检查覆盖所有必查项
