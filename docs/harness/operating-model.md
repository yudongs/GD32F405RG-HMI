# Operating Model（运行模型）

## Agent 边界

- 集成到项目之前，先生成或更新 Harness 草稿。
- 不要编造 build、test、flash、deployment 或 release 命令。
- 运行说明必须贴合扫描到的工具链和 workspace。

## 战略人控 vs 战术 AI

> 对应约束：`strategic-vs-tactical`。AI 更像战术执行者；**战略层**必须由人把控，否则坏结构会在好模型下被放大。

### 须由人决策并签字（战略）

- 架构分层与依赖方向（见 architecture-constraints）。
- 公开 API surface 与 interface-contracts。
- glossary 术语与对外命名。
- 测试金字塔分层与 gate 升级路径。

### 可在冻结接口与测试下交给 AI（战术）

- 在已冻结的公开类型/函数签名下实现模块内部逻辑。
- 使实现通过既有测试与本地 gate；再安全重构。
- 文档草稿与不改变已签字契约的重构。

## Review 模式

- 主要 review 模式：`human-review`。
- 安全边界：`standard`。
- 质量门槛：`balanced`。

## 项目专用说明

- **硬件验证**：使用 `mklink-flash` 技能进行烧录和调试
- **Build Tool**：Keil MDK-ARM (`UV4.exe`)
- **无 CI**：嵌入式项目无自动化 CI，所有变更依赖人工 review 和硬件验证
- **Grill 触发**：新增/修改外设驱动、修改中断处理、修改引脚复用时必须走 Grill 协议
