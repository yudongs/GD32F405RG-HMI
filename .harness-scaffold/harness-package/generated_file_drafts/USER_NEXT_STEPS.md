# 分步审核与集成指南 — GD32F405RG

## 起点

- Agent 入口草稿：`generated_file_drafts/HARNESS_INDEX.md`
- 仓库入口草稿（通用）：`generated_file_drafts/AGENTS.md`
- 仓库入口草稿（Claude Code 专用）：`generated_file_drafts/CLAUDE.md`
- 判断依据：`decision_log.md`
- 扫描摘要：`harness_summary.md`

## 步骤 1：审核扫描结论

- 核对 `harness_summary.md` 中的 profile、domain profiles、stage、workflow modes 和 safety boundary。
- 如果这些判断不符合项目事实，先修正访谈答案或扫描输入，再重新渲染 Harness 包。
- 不要在扫描结论明显错误时集成草稿。

## 步骤 2：审核生成草稿

- 先读 `generated_file_drafts/HARNESS_INDEX.md`，确认接手项目的 Agent 能从这里开始工作。
- 审核 **`generated_file_drafts/docs/harness/glossary.md`**（通用语言）与 **`generated_file_drafts/docs/harness/grill-protocol.md`**（Grill / 共享设计）：占位内容须在集成前替换为项目真实 owner、术语与触发门禁。
- 再读 `generated_file_drafts/AGENTS.md` 与 `generated_file_drafts/CLAUDE.md`，确认两个入口足够短、导航清晰、不重复深层文档；二者结构一致，仅 CLAUDE.md 末尾追加 Claude 专属提示，避免漂移。
- 检查 `generated_file_drafts/docs/harness/` 下的架构约束、接口契约、测试门禁、测试环境、工具计划和运行模型是否贴合项目。

## 步骤 3：确认 Gate 命令

- 构建 Gate: `cd . && <确认最小构建命令>` [placeholder, 项目评估]（需先确认）
- 最小测试 Gate: `cd . && <确认最小测试或冒烟命令>` [placeholder, 项目评估]（需先确认）
- Harness 文档一致性检查: `cd . && <确认文档一致性检查或审核清单>` [placeholder, Harness 策略]（需先确认）

- 只有高置信 phase-1 命令可以作为 required gate。
- placeholder 或 medium-confidence 命令必须继续保持待确认，直到项目 owner 明确确认。
- 不要编造 build、test、flash、deploy、signing 或 release 命令。

## 步骤 4：选择集成策略

- 如果项目没有任何 agent 入口：把 `generated_file_drafts/AGENTS.md` 与 `generated_file_drafts/CLAUDE.md` 一并放入仓库根目录，并把 `docs/harness/` 草稿放入项目文档目录。
- 如果项目已有 `AGENTS.md`、`CLAUDE.md` 或 `GEMINI.md`：逐个文件确认是合并、替代还是丢弃；保持入口规则与命令一致，避免多入口漂移。
- 推荐：把 AGENTS.md 作为唯一权威源，CLAUDE.md 仅做 Claude Code 专属提示 + 指向 AGENTS.md，减少维护成本。
- 如果 gate 或安全边界仍不确定：暂不应用，只保留 `.harness-scaffold/harness-package/` 作为审核草稿。
- **Grill 落地**：采用前须约定共享设计概念的输出位置（例如 `docs/plan/<feature>.md`、Issue 模板或 PR 检查清单），并与 `grill-protocol.md` 一致；若团队尚未约定，保持草稿不集成主仓，避免仅「改规格再生成」的 vibe loop。
- 不要直接覆盖现有项目文件；先确认写入文件清单、冲突点和替代关系。

## 步骤 5：应用后验证

- 运行 `generated_file_drafts/HARNESS_INDEX.md` 中列出的高置信 phase-1 验证命令。
- 检查入口文档中的链接是否都能打开。
- 把仍需人工确认的 gate、硬件步骤或发布边界记录在 PR/提交说明中。
