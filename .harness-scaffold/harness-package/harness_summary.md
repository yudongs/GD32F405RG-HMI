# Harness 摘要 — GD32F405RG

- Primary profile: `embedded-toolchain`
- Domain profiles: `embedded`
- Repo shape: `single-root`
- Stage: `existing`
- Workflow modes: `human-review`
- Languages: `c-header, c`
- Build systems: `unknown`
- Existing tools: `none detected`

## 主要信号

- No clear automated test signal was detected.
- No CI configuration was detected.
- No existing agent entrypoint or instruction file was detected.
- No high-confidence build command was detected.
- No high-confidence test command was detected.
- Embedded or native work needs explicit hardware versus CI validation boundaries.

## 默认假设

- Team mode：`human-review`
- Automation level：`generate-drafts`
- Security boundary：`standard`
- Quality bar：`balanced`

## Rollout 建议

- Phase 1：建立入口、确认构建 gate 和最小验证。
- Phase 2：增加风格、文档一致性和 profile-specific 检查。
- Phase 3：基线稳定后收紧 ratchet 规则。
