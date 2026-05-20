# 决策日志 — GD32F405RG

- Primary profile：`embedded-toolchain`
- Domain profiles: `embedded`
- Repo shape：`single-root`
- Maturity stage：`existing`
- Workflow modes: `human-review`
- 分类置信度：`medium`

## 证据

- `embedded_signal`: .ld, cmsis

## 约束决策

- `context-entrypoint` -> `required`: 未扫描到现有 agent 入口；Harness 需要生成一个清晰的入口草稿。
- `progressive-disclosure` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `architecture-boundaries` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `interface-contracts` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `file-size-complexity` -> `recommended`: 项目评估显示该约束适用于当前仓库形态。
- `test-gates` -> `required`: 未扫描到可靠命令，因此 gate 需要保留为待确认。
- `doc-freshness` -> `recommended`: 项目评估显示该约束适用于当前仓库形态。
- `agent-safety-boundaries` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `entropy-ratchet` -> `recommended`: 项目评估显示该约束适用于当前仓库形态。
- `acceptance-contract` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `driver-app-layering` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `board-specific-isolation` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `hardware-validation-path` -> `required`: 嵌入式证据要求区分 CI 可验证内容和硬件实测置信度。
- `toolchain-entrypoints` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `manual-verification-contract` -> `recommended`: 项目评估显示该约束适用于当前仓库形态。
- `module-cohesion` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `module-coupling` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `dependency-direction` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `public-api-surface` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `interface-versioning` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `test-pyramid` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `test-isolation` -> `required`: 项目评估显示该约束适用于当前仓库形态。
- `shared-design-concept` -> `required`: AI 辅助开发需先沉淀共享设计概念，避免仅改规格再生成的 vibe loop。
- `ubiquitous-language` -> `required`: 人与模型、业务与代码需共享同一套领域词汇表。
- `test-first-loop` -> `recommended`: 测试先行缩短反馈回路，约束模型不超越当前可验证范围。
- `deep-module` -> `required`: 对外保持简单曲面，复杂度收于模块内部，避免浅模块迷宫。
- `strategic-vs-tactical` -> `required`: 战略（架构/边界/语言/金字塔）须人签字；AI 只在冻结契约下战术实现。

## 工具决策

- `repo-scan` -> `skill-script`: 在提出高影响问题前先汇总仓库事实。
- `build-gate` -> `needs-confirmation`: 当仓库暴露构建命令时，保留确定性的构建 gate。
- `test-gate` -> `needs-confirmation`: 接受变更前定义最小可重复验证步骤。
- `doc-freshness-check` -> `needs-confirmation`: 保持入口文档与深层文档一致。
