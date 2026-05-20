# Tooling Plan（工具计划）

## 工具选型摘要

- `仓库扫描` (skill-script, phase-0): 在提出高影响问题前先汇总仓库事实。
- `构建 Gate` (user-confirmed, phase-1): Keil MDK-ARM 编译 gate。
- `Hardware Validation` (skill, phase-1): 使用 mklink-flash 技能进行硬件验证。
- `Harness 文档一致性检查` (needs-confirmation, phase-2): 保持入口文档与深层文档一致。

## 命令

### 仓库扫描
- `python scripts/project_scan.py <repo> --output .harness-scaffold/project_facts.json` [high, skill-script]

### 构建 Gate
- `UV4.exe -j0 -db -path "Project" "Project\GD32F405RG.uvprojx"` [high, user confirmed]

### Hardware Validation（mklink-flash 技能）
- `python -m mklink project-init` — 初始化项目配置 [high]
- `python -m mklink flash` — 烧录固件 [high]
- `python -m mklink rtt --duration 10` — RTT 输出 [high]
- `python -m mklink hardfault --source <axf>` — HardFault 分析 [high]
- `python -m mklink read-reg <寄存器>` — 读取寄存器 [high]

### Harness 文档一致性检查
- 人工审核：使用 `docs/harness/REVIEW_CHECKLIST.md` [manual]
- Phase-2 考虑：自动化脚本检查文档同步

## Rollout

- Phase 1：建立入口、确认构建 gate 和硬件验证。
- Phase 2：收紧文档一致性和 profile-specific 检查。
- Phase 3：证据稳定后再升级 recommended 或 placeholder 规则。
