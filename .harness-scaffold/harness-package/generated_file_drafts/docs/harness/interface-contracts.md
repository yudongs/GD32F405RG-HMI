# Interface Contracts（接口契约）

> 跨模块共享的接口、Schema、HTTP 契约清单。任何变更前先查这里。

## 契约清单

| 契约 ID | 类型 | 位置 | Owner | 消费者 | 变更流程 |
|---|---|---|---|---|---|
| `<contract-id>` | `<type/schema/http/event/binary>` | `<path>` | `<owner>` | `<consumers>` | `<flow>` |

> 类型：`type`（TypeScript/Python 类型）/ `schema`（Zod/JSON Schema/SQL DDL）/ `http`（REST/RPC）/ `event`（消息/Webhook）/ `binary`（协议帧）。

## 变更流程

- 1. 修改契约定义文件（schema/类型/proto）。
- 2. 同 PR 修改实现 + 调用方。
- 3. 在 `decision_log.md` 留痕。
- 4. 跨模块契约需 owner 列表全员 review。

> 默认流程：(1) 修改契约定义文件；(2) 同 PR 修改实现 + 调用方；(3) 在 `decision_log.md` 记录；(4) 跨团队契约需 owner 列表全员 review。

## Breaking-Change 协议

- PR 标题加 `BREAKING:` 前缀。
- 保留旧字段 / 旧路由至少一个发布周期。
- 在契约文件中标注 `@deprecated` 与替代项。

> 默认：breaking change 需要在 PR 标题加 `BREAKING:` 前缀；保留旧字段一个发布周期；新旧并存期间在契约文件里标 `@deprecated`。

## 公开 API Surface 一览

- 请逐模块列出公开入口与导出标识符。

> 每个模块只暴露一个入口文件（`index.ts`/`__init__.py`/barrel）。本节列出每个模块的公开入口和它对外导出的标识符；未在此列出的内部实现禁止跨模块导入。

## 已知契约债务

- 暂无已知契约债务。
