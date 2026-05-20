# Testing And Gates（测试与门禁）

> 测试是 Harness 的"反馈回路"。本文档定义金字塔分层、命令、覆盖率底线与升级路径。
> 配套环境与外部边界 mock 见 [测试环境](test-environment.md)。

## 测试金字塔

| 层级 | 必备性 | 范围 | 命令 | 置信度 | 备注 |
|---|---|---|---|---|---|
| Build Gate | required | Keil 编译 | `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` | high | 验证代码可编译 |
| Manual Smoke | required | 硬件验证 | 人工烧录 + 功能验证 | manual | 嵌入式无自动化测试，依赖人工验证 |
| Hardware Validation | recommended | 硬件 in-circuit | `python -m mklink flash` + `mklink rtt/hardfault` | high | 使用 mklink-flash 技能 |

## 构建 Gate（Phase-1）

| 命令 | 置信度 | 备注 |
|---|---|---|
| `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` | high | 用户确认使用 Keil MDK-ARM；需确认 UV4.exe 在 PATH 中 |

## 硬件验证命令（mklink-flash 技能）

| 命令 | 用途 | 置信度 |
|------|------|--------|
| `python -m mklink project-init` | 初始化项目配置 | high |
| `python -m mklink flash` | 烧录固件 | high |
| `python -m mklink rtt --duration 10` | 读取 RTT 输出 | high |
| `python -m mklink hardfault --source <axf>` | HardFault 分析 | high |
| `python -m mklink read-reg <寄存器>` | 读取寄存器 | high |

## 覆盖率底线

嵌入式裸机项目无自动化测试覆盖率指标：
- 代码审查替代自动化覆盖率检查
- 人工 smoke test 验证关键路径
- 关键外设驱动（RCU/GPIO/ADC）需主程 review

## Acceptance Contract（验收契约）

- Build Gate 必须通过：Keil 编译无错误
- Manual Smoke 必须通过：烧录后功能正常
- 由人工 reviewer 决定是否接受变更
- 硬件相关变更（外设驱动/中断）需主程 review
