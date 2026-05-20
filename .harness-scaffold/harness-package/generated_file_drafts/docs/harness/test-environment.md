# Test Environment（测试环境）

> 嵌入式裸机项目的测试环境定义。配套金字塔与命令见 [测试与门禁](testing-and-gates.md)。

## 测试分层

| 层级 | 环境 | 说明 |
|------|------|------|
| Build Gate | CI/本地 | Keil 编译通过，不依赖硬件 |
| Manual Smoke | 硬件 | 烧录后人工验证功能 |
| Hardware Validation | 硬件+调试器 | 使用 mklink-flash 技能进行硬件 in-circuit 验证 |

## 外部边界 Mock 策略

嵌入式裸机项目无 OS、无文件系统、无网络：
- **无 Mock 需要**：直接操作寄存器，无外部依赖
- **外设隔离**：使用示波器/逻辑分析仪观测硬件行为
- **时间/随机源**：使用固定 seed 或 fake timer（如果 RTOS 支持）

## 硬件验证环境

使用 `mklink-flash` 技能进行硬件验证：

| 命令 | 用途 |
|------|------|
| `python -m mklink project-init` | 初始化项目配置 |
| `python -m mklink flash` | 烧录固件到芯片 |
| `python -m mklink rtt` | 读取 RTT 输出 |
| `python -m mklink read-reg <reg>` | 读取寄存器 |
| `python -m mklink hardfault --source <axf>` | HardFault 分析 |

## 故障定位

- 编译失败：检查 Keil 输出窗口错误信息
- 烧录失败：检查 SWD 接线和目标板供电
- 运行时故障：使用 `mklink hardfault` 读取 Cortex-M Fault 寄存器
- 行为异常：使用 `mklink rtt` 或 `mklink superwatch` 观测运行时状态
