# Harness Index — GD32F405RG

> 本文件为本项目上工作的任意 AI agent 的入口。
> 请先读本文。仅在需要细节时再打开链接文档。

## Quick Facts

| Key | Value |
|-----|-------|
| Profile | `embedded-toolchain` |
| Domains | `embedded` |
| Repo shape | `single-root` |
| Workflow modes | `human-review` |
| Stage | `existing` |
| Languages | `c-header, c` |
| Build systems | `Keil MDK-ARM` |

## 何时读何文档

用下表决定打开哪份文档。不要一次性读完所有内容 — 按需加载。

| 当你需要…… | 阅读 |
|---------------------|-----------|
| 了解 agent 可做与不可做 | [运行模型](docs/harness/operating-model.md) |
| 改代码前核对架构规则 | [架构约束](docs/harness/architecture-constraints.md) |
| 用 Grill 对齐共享设计概念（分支/依赖/失败模式） | [Grill 协议](docs/harness/grill-protocol.md) |
| 查领域术语、统一人与 AI 用词 | [通用语言表](docs/harness/glossary.md) |
| 运行或补充 build/test gates | [测试与门禁](docs/harness/testing-and-gates.md) |
| 选择或配置工具 | [工具计划](docs/harness/tooling-plan.md) |
| 提交前审核文档一致性 | [审核清单](docs/harness/REVIEW_CHECKLIST.md) |
| 查看用户下一步 | [下一步](USER_NEXT_STEPS.md) |
| 查看判断依据 | `decision_log.md` |
| 查看项目评估 | `project_assessment.json` |
| 查看完整 constraint 矩阵（机器可读） | `constraints_matrix.json` |
| 查看完整工具选型（机器可读） | `tooling_selection.json` |
| 查看脚手架生成清单 | `scaffold_manifest.json` |

## 不可协商的规则（始终生效）

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

> **战略人控**：架构、模块边界、通用语言（`glossary.md`）与测试金字塔须由人决策与签字；AI 仅在已冻结的公开接口与测试契约下做战术实现。详见 [运行模型](docs/harness/operating-model.md)。

## 验证命令（接受变更前运行）

- Keil 构建: `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` [high confidence, user confirmed]

## 需确认后才能升级为 Gate 的命令

- 硬件验证: `python -m mklink flash` / `mklink rtt` [mklink-flash skill]
- Harness 文档一致性检查: 使用 [审核清单](docs/harness/REVIEW_CHECKLIST.md) [manual]

## 已知陷阱

- 无 CI 配置：所有变更依赖人工 review
- 无自动化测试：关键路径依赖人工 smoke test
- 硬件验证必须使用 mklink-flash 技能进行 in-circuit 验证

## Rollout 阶段

- Phase 1：建立入口、确认构建 gate 和硬件验证（mklink-flash）。
- Phase 2：收紧文档一致性和 profile-specific 检查（引入审核清单）。
- Phase 3：证据稳定后再升级 recommended 或 placeholder 规则。

---

*由 harness-scaffolder 生成。完整扫描报告见 `harness_summary.md`。*
