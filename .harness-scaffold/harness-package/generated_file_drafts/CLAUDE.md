# CLAUDE.md

> Claude Code (`claude.ai/code`) 在本仓库工作时的入口文件。
> 与 `AGENTS.md` 共享同一组规则与命令；如二者出现差异，以 [`docs/harness/HARNESS_INDEX.md`](docs/harness/HARNESS_INDEX.md) 为准。

## 项目快照

- 项目名称：`GD32F405RG`
- Primary profile：`embedded-toolchain`
- Domain profiles：`embedded`
- Repo shape：`single-root`
- Workflow modes：`human-review`
- Stage：`existing`
- 语言：`c-header, c`
- Build system：`unknown`

## 不可协商的约束

- Maintain one short agent entrypoint for the repo. (`required`)
- Keep detailed guidance in docs/harness instead of one large instruction blob. (`required`)
- Define module, subsystem, and dependency boundaries explicitly. (`required`)
- Centralize shared interfaces, schemas, or boundary contracts. (`required`)
- Separate drivers, BSP, and application logic. (`required`)
- Isolate board and MCU specifics from reusable logic. (`required`)
- Each file and module owns one concern; sibling files share the same purpose. (`required`)
- Forbid back-references and circular imports between modules. (`required`)
- Declare layer order (e.g. UI -> service -> repo -> infra); lower layers must not import higher layers. (`required`)
- Each module exposes a single public entrypoint (e.g. index.ts, __init__.py); internals are not imported across modules. (`required`)
- Shared interfaces and schemas have an owner, change process, and breaking-change protocol. (`required`)
- Before material feature work, use a Grill protocol to align the invisible shared design concept (branches, deps, failure modes); output lands in PRD/task/issue. (`required`)
- Domain terms live in docs/harness/glossary.md; requirements, code names, and discussions use the same vocabulary. (`required`)
- Prefer deep modules: simple public surface, complexity inside; avoid shallow maze of tiny files and wrappers with complex call graphs. (`required`)

## 验证命令（接受变更前必跑）

- Keil 构建: `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` [high confidence, user confirmed]

## 需确认才能升级为 Gate 的命令

- 最小测试 Gate: `cd . && <确认最小测试或冒烟命令>` [placeholder, 项目评估]（需先确认）
- Harness 文档一致性检查: `cd . && <确认文档一致性检查或审核清单>` [placeholder, Harness 策略]（需先确认）

## 写代码前（基本功）

- 对齐共享设计：按 [`docs/harness/grill-protocol.md`](docs/harness/grill-protocol.md) 追问分支、依赖、失败模式与回滚，并沉淀 PRD/任务单或 Issue。
- 核对术语：[`docs/harness/glossary.md`](docs/harness/glossary.md)；需求、命名与讨论须与本表一致。
- 对 [`docs/harness/testing-and-gates.md`](docs/harness/testing-and-gates.md) 中列出的 test-first 目标路径：先写测试、再让实现通过、再重构。

## 工作规则

- 团队模式：`human-review`。
- 自动化级别：`generate-drafts`。
- 修改目标仓库前，先展示 Harness 草稿文件。
- 没有用户确认时，不要把 placeholder 命令升级为 required gate。
- 网络搜索只用于公开工具/框架确认，不泄露私有源码。

## Claude Code 专用提示

- 编辑前必须先用 Read 工具读取目标文件，禁止盲改。
- 禁止凭记忆写支付/数据库相关代码；不确定的契约先查 [`docs/harness/interface-contracts.md`](docs/harness/interface-contracts.md)。
- 触碰 `lib/payment/`、`lib/db/` 时必须补对应 unit 测试，参考 [`docs/harness/testing-and-gates.md`](docs/harness/testing-and-gates.md)。
- 任何跨模块 import 必须走公开入口（`@/lib/<module>`），禁止深 import。

## Harness 文档

- Harness 索引：[`docs/harness/HARNESS_INDEX.md`](docs/harness/HARNESS_INDEX.md)
- 架构约束：[`docs/harness/architecture-constraints.md`](docs/harness/architecture-constraints.md)
- Grill 协议：[`docs/harness/grill-protocol.md`](docs/harness/grill-protocol.md)
- 通用语言：[`docs/harness/glossary.md`](docs/harness/glossary.md)
- 接口契约：[`docs/harness/interface-contracts.md`](docs/harness/interface-contracts.md)
- 测试与门禁：[`docs/harness/testing-and-gates.md`](docs/harness/testing-and-gates.md)
- 测试环境：[`docs/harness/test-environment.md`](docs/harness/test-environment.md)
- 工具计划：[`docs/harness/tooling-plan.md`](docs/harness/tooling-plan.md)
- 运行模型：[`docs/harness/operating-model.md`](docs/harness/operating-model.md)

## 已知陷阱

- No clear automated test signal was detected.
- No CI configuration was detected.
- No existing agent entrypoint or instruction file was detected.
- No high-confidence build command was detected.
- No high-confidence test command was detected.
- Embedded or native work needs explicit hardware versus CI validation boundaries.
