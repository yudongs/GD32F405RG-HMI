# Architecture Constraints（架构约束）

## 范围

本文档记录 `GD32F405RG` 的初始 Harness constraints。

## Profile

- Primary profile：`embedded-toolchain`
- Stage：`existing`

## 分层依赖图

```mermaid
flowchart LR
  App["application"] --> Middle["middleware / RTOS hooks"]
  Middle --> Drv["drivers / HAL"]
  Drv --> Bsp["BSP / board"]
```

> 箭头方向 = 允许的 import 方向。任何反向 import（低层 -> 高层）或同层跨模块深 import 视作违反 `dependency-direction` 与 `module-coupling`。

## 模块职责（高内聚）

| 模块 | 职责 | 公开入口 |
|------|------|----------|
| `CMSIS/` | ARM CMSIS 标准头文件、内核访问、启动文件 | `core_cm4.h`, `system_gd32f4xx.h` |
| `CMSIS/GD/GD32F4xx/Source/GCC/Ld/` | 链接脚本（Flash/RAM 布局） | `*.ld` |
| `GD32_StdPeriph_Driver/` | 标准外设驱动库（GPIO/ADC/USART/SPI/I2C/DMA 等） | `gd32f4xx.h` |
| `User/` | 应用层入口 | `main.c` |
| `APP/` | 应用配置（如 LCD 端口配置） | `lcd_port.h` |
| `Project/` | Keil 工程文件 | `*.uvprojx` |

> 同一模块内的文件必须共享单一关注点。驱动层（StdPeriph）禁止调用应用层代码。

## 公开 API Surface（低耦合）

- 请逐模块列出公开入口文件（`index.ts`/`__init__.py`/barrel），未列出的视作内部实现。

> 跨模块导入只能从模块的公开入口（`index.ts`/`__init__.py`/barrel）走；禁止 `import { x } from 'lib/payment/wechat/internal/foo'` 这类深路径。

## 深模块（小接口、大内涵）

> 对应约束：`deep-module`。目标：对外 **简单、稳定** 的曲面；复杂性与分支 **藏在模块内部**。大语言模型易把项目写成大量零碎小文件，导致调用方在依赖图中迷路——须显式拒绝「浅模块迷宫」。

- 由人补充：`interview_answers.deep_modules`（module、public_surface、frozen_by、internals_owned_by）。
- 默认：业务子系统应有稳定、窄的公开入口；禁止大量仅转发调用的浅文件链。

**反例（浅模块 / 应避免）**：

- 大量文件只做薄封装转发，调用链深但每文件几行，接口总表面积膨胀。
- 公开 API 曲面由人频繁改动，内部却未收敛复杂度。
- 为「文件数」而拆模块，违反高内聚。

**正例（深模块）**：

- 公开入口少、签名稳定；模块内部可自由拆分私有子文件，只要不暴露给其他模块。
- 跨模块只依赖公开类型与文档化契约；Review 时优先审视 **公开面** 是否仍简单。

## 反向依赖与循环

- 禁止反向依赖（低层 import 高层）。
- 禁止跨模块深 import（绕过公开入口）。
- 循环依赖检查命令：`<待确认>`（建议 madge/pydeps，phase-2 升 required）。

## Constraint 集合

### `context-entrypoint`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Maintain one short agent entrypoint for the repo.
- 执行方式：AGENTS.md, doc review
- 验证提示：AGENTS.md exists and points to deeper docs instead of duplicating them.
- 判断依据：未扫描到现有 agent 入口；Harness 需要生成一个清晰的入口草稿。

### `progressive-disclosure`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Keep detailed guidance in docs/harness instead of one large instruction blob.
- 执行方式：docs/harness, doc freshness
- 验证提示：Detailed rules live below docs/harness and the entrypoint stays short.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `architecture-boundaries`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Define module, subsystem, and dependency boundaries explicitly.
- 执行方式：architecture doc, review checklist
- 验证提示：Architecture constraints name the repo-specific subsystem boundaries.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `interface-contracts`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Centralize shared interfaces, schemas, or boundary contracts.
- 执行方式：shared contract doc, type or schema checks
- 验证提示：Shared contracts have an owner and an expected validation path.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `file-size-complexity`
- 级别：`recommended`
- 类别：`computational-feedback`
- 摘要：Prevent files and functions from growing beyond reviewable size.
- 执行方式：lint threshold, review checklist
- 验证提示：Adoption can start as review guidance and later become a lint gate.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `test-gates`
- 级别：`required`
- 类别：`computational-feedback`
- 摘要：Define the minimum checks before work is accepted.
- 执行方式：build gate, test gate, smoke checks
- 验证提示：Required gates only contain high-confidence commands or explicitly confirmed commands.
- 判断依据：未扫描到可靠命令，因此 gate 需要保留为待确认。

### `doc-freshness`
- 级别：`recommended`
- 类别：`computational-feedback`
- 摘要：Keep agent entrypoints and Harness docs synced with the repo.
- 执行方式：freshness script, review checklist
- 验证提示：Harness docs mention the current toolchain and known entrypoints.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `agent-safety-boundaries`
- 级别：`required`
- 类别：`operating-model`
- 摘要：State what agents may edit, run, or only propose.
- 执行方式：operating model doc
- 验证提示：The operating model defines write, execution, network, and deployment boundaries.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `entropy-ratchet`
- 级别：`recommended`
- 类别：`governance`
- 摘要：Prevent new work from increasing structural entropy.
- 执行方式：allowlist, phased tightening
- 验证提示：Legacy or mixed repos define which areas are ratcheted first.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `acceptance-contract`
- 级别：`required`
- 类别：`inferential-feedback`
- 摘要：Explain how proposed work is accepted, rejected, or escalated.
- 执行方式：review checklist, reviewer loop
- 验证提示：The Harness states the reviewer model and what evidence is required.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `driver-app-layering`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Separate drivers, BSP, and application logic.
- 执行方式：directory rules, review checklist
- 验证提示：Driver and application ownership boundaries are explicit.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `board-specific-isolation`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Isolate board and MCU specifics from reusable logic.
- 执行方式：build targets, include rules
- 验证提示：Board-specific code has a named location and cannot leak into shared logic.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `hardware-validation-path`
- 级别：`required`
- 类别：`computational-feedback`
- 摘要：Define what is validated in CI versus on hardware.
- 执行方式：build matrix, bench checklist
- 验证提示：Hardware-only validation is clearly separated from CI gates.
- 判断依据：嵌入式证据要求区分 CI 可验证内容和硬件实测置信度。

### `toolchain-entrypoints`
- 级别：`required`
- 类别：`operating-model`
- 摘要：Document build, flash, debug, and recovery entrypoints.
- 执行方式：operating model doc
- 验证提示：Unknown flash/debug commands remain placeholders until confirmed.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `manual-verification-contract`
- 级别：`recommended`
- 类别：`inferential-feedback`
- 摘要：State manual validation expectations when automation is limited.
- 执行方式：review checklist, test notes
- 验证提示：Manual checks include owner, environment, and pass/fail evidence.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `module-cohesion`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Each file and module owns one concern; sibling files share the same purpose.
- 执行方式：architecture doc, review checklist, directory README
- 验证提示：Architecture doc lists each module's single responsibility and rejects mixed concerns.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `module-coupling`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Forbid back-references and circular imports between modules.
- 执行方式：import lint, circular-dependency check, review checklist
- 验证提示：A circular-dependency check command is named (or marked placeholder) and runs in CI when adopted.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `dependency-direction`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Declare layer order (e.g. UI -> service -> repo -> infra); lower layers must not import higher layers.
- 执行方式：architecture doc, import lint
- 验证提示：Architecture doc shows a layered dependency diagram and forbidden directions.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `public-api-surface`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Each module exposes a single public entrypoint (e.g. index.ts, __init__.py); internals are not imported across modules.
- 执行方式：architecture doc, barrel files, import lint
- 验证提示：The Harness names each module's public entrypoint and forbids deep imports across module boundaries.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `interface-versioning`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Shared interfaces and schemas have an owner, change process, and breaking-change protocol.
- 执行方式：interface-contracts doc, review checklist
- 验证提示：Each shared contract lists owner, location, and how breaking changes are introduced.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `test-pyramid`
- 级别：`required`
- 类别：`computational-feedback`
- 摘要：Define explicit unit/integration/e2e layers with minimum scope and command per layer.
- 执行方式：testing-and-gates doc, test layout
- 验证提示：Testing doc lists each layer, what it covers, its command, and confidence.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `test-isolation`
- 级别：`required`
- 类别：`computational-feedback`
- 摘要：Tests must not depend on real network, real credentials, or production data; external boundaries are mocked or sandboxed.
- 执行方式：test-environment doc, review checklist
- 验证提示：Test environment doc names every external boundary and how it is mocked or sandboxed.
- 判断依据：项目评估显示该约束适用于当前仓库形态。

### `shared-design-concept`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Before material feature work, use a Grill protocol to align the invisible shared design concept (branches, deps, failure modes); output lands in PRD/task/issue.
- 执行方式：grill-protocol doc, PR template, review checklist
- 验证提示：grill-protocol.md documents trigger, owner, question list, output path, and done-definition; work is traceable to that artifact.
- 判断依据：AI 辅助开发需先沉淀共享设计概念，避免仅改规格再生成的 vibe loop。

### `ubiquitous-language`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Domain terms live in docs/harness/glossary.md; requirements, code names, and discussions use the same vocabulary.
- 执行方式：glossary doc, review checklist
- 验证提示：Glossary lists term, definition, owner, and forbidden synonyms; changes flow glossary-first then code/docs.
- 判断依据：人与模型、业务与代码需共享同一套领域词汇表。

### `test-first-loop`
- 级别：`recommended`
- 类别：`computational-feedback`
- 摘要：For agreed target dirs: write tests first, make implementation pass, then refactor (TDD-style) to shorten the AI feedback loop.
- 执行方式：testing-and-gates doc, review checklist
- 验证提示：testing-and-gates.md lists test_first_targets; PRs touching those paths show test-before-implementation or escalation note.
- 判断依据：测试先行缩短反馈回路，约束模型不超越当前可验证范围。

### `deep-module`
- 级别：`required`
- 类别：`feedforward`
- 摘要：Prefer deep modules: simple public surface, complexity inside; avoid shallow maze of tiny files and wrappers with complex call graphs.
- 执行方式：architecture doc, public-api-surface lint, review checklist
- 验证提示：architecture-constraints.md states frozen public surfaces per module and forbids shallow-wrapper-only splits.
- 判断依据：对外保持简单曲面，复杂度收于模块内部，避免浅模块迷宫。

### `strategic-vs-tactical`
- 级别：`required`
- 类别：`operating-model`
- 摘要：Humans own strategy: architecture, boundaries, ubiquitous language, test pyramid; AI acts tactically inside frozen interfaces and tests.
- 执行方式：operating model doc, review checklist
- 验证提示：operating-model.md lists strategic_human_gates vs tactical AI tasks; no new cross-module contracts without human sign-off.
- 判断依据：战略（架构/边界/语言/金字塔）须人签字；AI 只在冻结契约下战术实现。

## Ratchet 说明

- 暂无记录。
