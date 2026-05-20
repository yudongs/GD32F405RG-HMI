# AGENTS.md

> `GD32F405RG` 的 AI agent 入口。
> Primary profile：`embedded-toolchain`
> Stage：`existing`

## 项目快照

- 项目名称：`GD32F405RG`
- Primary profile：`embedded-toolchain`
- Domain profiles：`embedded`
- Repo shape：`single-root`
- Workflow modes：`human-review`
- Stage：`existing`
- 语言：`c-header, c`
- Build system：`Keil MDK-ARM`

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

## 验证命令

- Keil 构建: `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` [high confidence, user confirmed]
- 硬件烧录: `python -m mklink flash` [high, mklink-flash skill]
- 硬件验证: `python -m mklink rtt --duration 10` [high, mklink-flash skill]

## 需确认命令

- Harness 文档一致性检查: 使用 [审核清单](docs/harness/REVIEW_CHECKLIST.md) [manual]

## 写代码前（基本功）

- 对齐共享设计：按 [Grill 协议](docs/harness/grill-protocol.md) 追问分支、依赖、失败模式与回滚，并沉淀 PRD/任务单或 Issue。
- 核对术语：[通用语言表](docs/harness/glossary.md)；需求、命名与讨论须与本表一致。
- 对 [测试与门禁](docs/harness/testing-and-gates.md) 中列出的 test-first 目标路径：先写测试、再让实现通过、再重构。

## 工作规则

- 团队模式：`human-review`。
- 自动化级别：`generate-drafts`。
- 修改目标仓库前，先展示 Harness 草稿文件。
- 没有用户确认时，不要把 placeholder 命令升级为 required gate。
- 网络搜索只用于公开工具/框架确认，不泄露私有源码。

## Harness 文档

- Architecture constraints：`docs/harness/architecture-constraints.md`
- Grill protocol（共享设计）：`docs/harness/grill-protocol.md`
- Glossary（通用语言）：`docs/harness/glossary.md`
- Interface contracts：`docs/harness/interface-contracts.md`
- Testing and gates：`docs/harness/testing-and-gates.md`
- Test environment：`docs/harness/test-environment.md`
- Tooling plan：`docs/harness/tooling-plan.md`
- Operating model：`docs/harness/operating-model.md`
- 审核清单：`docs/harness/REVIEW_CHECKLIST.md`

## 已知陷阱

- 无 CI 配置：所有变更依赖人工 review
- 无自动化测试：关键路径依赖人工 smoke test
- 硬件验证必须使用 mklink-flash 技能进行 in-circuit 验证
