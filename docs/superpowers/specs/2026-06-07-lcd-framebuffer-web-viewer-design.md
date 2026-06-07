# LCD Framebuffer Web Viewer — Design Spec

**Date:** 2026-06-07
**Status:** Draft (pending user review)
**Author:** Claude (brainstorming session)

## Goal

把 `framebuffer` 数组（`APP/lcd_st7586.c:18`，240×160 @ 1bpp，4800 字节，运行时地址 `0x20001ba4`）通过 `dump-memory` 持续采样并实时显示到浏览器，支撑开发调试期间的远程"看屏"。

## Non-Goals (YAGNI)

- 多人/权限系统：单用户 localhost 工具
- 远程（LAN/WAN）访问：默认 127.0.0.1，需要时改 `--host`
- 多 framebuffer 区域：只读 `framebuffer` 单一区域
- 录制 / 回放：实时流，不存盘（除手动保存当前帧）
- MCP 集成：`mcp_server.py` 已有 `dump_framebuffer` 走 VTFPAgent，本工具走 dump-memory，独立路径
- 写操作（按键注入、清屏）：只读

## Architecture

3 层独立进程，2 个清晰接口：

```
┌──────────────────────────────────────────────────────────┐
│  Browser (tools/lcd_viewer/index.html)                   │
│  - WebSocket client  → ws://127.0.0.1:8765/ws            │
│  - 240×160 <canvas>  (image-rendering: pixelated)        │
│  - 控件：⏸ 暂停 / ▶ 继续 / 周期[N]ms / 💾 保存           │
└──────────────────┬───────────────────────────────────────┘
                   │  WebSocket (binary 4800B frames + JSON ctrl)
                   ▼
┌──────────────────────────────────────────────────────────┐
│  WebSocket Server (tools/lcd_viewer/server.py)           │
│  - websockets 库 + asyncio                                │
│  - 静态文件服务：/index.html, /app.js                     │
│  - WS endpoint: /ws                                      │
│  - 客户端引用计数：count == 0 → 杀子进程                 │
└──────────────────┬───────────────────────────────────────┘
                   │  Popen(stdout=PIPE, text=True)  读 JSON 行
                   ▼
┌──────────────────────────────────────────────────────────┐
│  dump-memory 子进程 (mklink CLI)                         │
│  python -m mklink dump-memory 0x20001ba4:4800 \          │
│      --period 0.05 --frames 0 --duration 0 --json        │
│  - stdout 每帧一行 JSON                                   │
│  - 包含 ts_us, blocks[].payload_b64                       │
└──────────────────────────────────────────────────────────┘
```

## File Structure

```
tools/lcd_viewer/
├── __init__.py
├── server.py              # 入口：python tools/lcd_viewer/server.py
├── ws_server.py           # WebSocket 服务 + 静态文件
├── dumper.py              # dump-memory 子进程封装（启动/停止/读 JSON 行）
├── frame.py               # 数据类：Frame(ts_us, payload) + 解析 + 校验
├── config.py              # 常量：FB_ADDR/FB_SIZE/DEFAULT_PERIOD/PORT
├── README.md              # 启动方式 / 排错
└── web/
    ├── index.html         # 单一页面（无外部依赖）
    └── app.js             # WS 客户端 + Canvas 渲染 + 控件逻辑
```

### 职责

| 文件 | 职责 | 不做什么 |
|------|------|----------|
| `server.py` | CLI 入口、参数解析、装配 | 业务逻辑 |
| `ws_server.py` | 监听 WS、广播帧、HTTP 静态文件、维护客户端计数 | 不解析 dump-memory 输出 |
| `dumper.py` | 启停子进程、读 stdout JSON 行、回调给 ws_server | 不懂 WS |
| `frame.py` | 把 `blocks[]` 合并成单一 `bytes(4800)`、校验、提取 ts | 不读 stdin |
| `config.py` | 所有常量集中 | 业务 |
| `web/app.js` | WS 连接、canvas 渲染、控件、状态栏 | 不直接读 mcu |

### 依赖方向（严格单向，禁止反向 import）

```
server.py
   ↓
ws_server.py ←→ dumper.py
                  ↓
                frame.py
                  ↓
               config.py
```

`web/` 目录完全独立：纯前端，无构建步骤。

## Key Decisions (from brainstorming)

| 维度 | 选择 |
|------|------|
| 页面布局 | D — 顶部状态栏、底部控件（开始/暂停/周期/保存）|
| 显示风格 | D — 1:1 真实像素（240×160，pixelated），深色 + 亮绿像素 |
| 流协议 | WebSocket + 二进制帧（client→server 双向）|
| 文件位置 | `tools/lcd_viewer/` 新目录 |
| 采集方式 | `subprocess` 调 `python -m mklink dump-memory ... --json` |
| 网络范围 | 仅 `127.0.0.1` |
| WS 框架 | `websockets` 库（16.0 已装）|
| 保存格式 | PNG（240×160 黑白，PIL 已装）|
| 进程生命周期 | 首个客户端连上才启动；最后一个退出后停止 |
| 端口 | 8765 |
| 周期 | 50ms 默认，UI 可改（10~200ms 范围）|

## Data Flow

### 启动

```
1. python tools/lcd_viewer/server.py
   └─→ 加载 config.py
   └─→ 启动 asyncio event loop
       ├─ HTTP server (127.0.0.1:8765)  ── 提供 index.html / app.js
       └─ WS server   (127.0.0.1:8765/ws)  ── 监听客户端
   └─ 打印 "Server: http://127.0.0.1:8765"
   └─ 等待客户端
```

### 客户端连接 → 子进程启动

```
Browser                                 Server                           dump-memory
  │  WS upgrade /ws                       │                                  │
  ├──────────────────────────────────────►│  client_count: 0→1              │
  │                                       │  spawn subprocess                │
  │                                       ├─────────────────────────────────►│
  │                                       │                                  │ (启动 ~150ms)
  │  ◄── text: {"type":"hello","fb_size":4800,"period_ms":50}              │
  │  ◄── binary: 4800 bytes               │  ◄── {"ts_us":..,"blocks":[..]}   │
  │  ◄── binary: 4800 bytes               │  ◄── {"ts_us":..,"blocks":[..]}   │
  │  ...                                                                     │
```

### 客户端断开

```
Browser 关闭 ──► WS close ──► server 收到 disconnect
                                    │
                                    ├─ client_count 减 1
                                    └─ 若 == 0：
                                         ├─ 发 RTTView.stop() 给设备
                                         ├─ 杀子进程
                                         └─ 打印 "[dumper] idle, subprocess terminated"
```

下一个客户端再连时，重复"启动"序列。

## WebSocket Protocol

### Server → Client 文本帧（JSON）

| 消息 | 含义 |
|------|------|
| `{"type":"hello", "fb_w":240, "fb_h":160, "period_ms":50, "port":"COM4"}` | 首次连接握手 |
| `{"type":"status", "state":"running"\|"starting"\|"stopped"\|"error", "msg":"..."}` | 状态变化 |
| `{"type":"stats", "fps":18.7, "frames":1234, "ts_us":204119544}` | 每秒一次统计 |

### Server → Client 二进制帧

固定 4800 字节（按 `FB_SIZE` 校验），无任何头。每帧直接对应 framebuffer 一帧。

### Client → Server 文本帧（JSON）

| 消息 | 含义 |
|------|------|
| `{"type":"set_period", "ms":30}` | 改采样周期 → server 杀子进程后用新周期重启 |
| `{"type":"pause"}` / `{"type":"resume"}` | 客户端渲染暂停（不杀子进程）|
| `{"type":"save_png"}` | 请求保存 → server 把当前最新帧以 PNG 流回 |

### 保存 PNG 流程

```
Browser:  点击 💾
   │
   ▼
Server: 收到 {"type":"save_png"}
   │
   ├─ 取 latest_frame.payload (4800B)
   ├─ PIL 转 240×160 mode='1' (黑/白两色)
   ├─ io.BytesIO 编码 PNG
   └─ binary 帧回发: [4B len][PNG bytes]
              │
              ▼
Browser: 触发 <a download="frame_<ts>.png">
```

## State Machine

```
         (server start)
              │
              ▼
        ┌──────────┐
        │  IDLE    │  无客户端，子进程未启
        └────┬─────┘
             │ 1st client connect
             ▼
        ┌──────────┐
        │ STARTING │  spawn dump-memory (~150ms)
        └────┬─────┘
             │ first frame received
             ▼
        ┌──────────┐
        │ RUNNING  │  ◄──┐
        └────┬─────┘     │ set_period
             │           │
             ├───────────┘
             │ last client disconnect
             ▼
        ┌──────────┐
        │ STOPPING │  RTTView.stop() + kill
        └────┬─────┘
             │
             ▼
        ┌──────────┐
        │  IDLE    │
        └──────────┘

 任何状态 ── subprocess 非 0 退出 ──► ERROR
                                       │
                                       │ 下个客户端连上时
                                       └─► STARTING (重试)
```

## Error Handling

| 故障 | 检测 | 行为 |
|------|------|------|
| MCU 离线 / 串口拔 | `dump-memory` 子进程退出码非 0 | server 捕获，置 `state="error"`，推 status 消息给客户端，**不**自动重启 |
| dump-memory 偶发 `flush fail` | stdout 含 `flush fail` 行 | 由 `mklink` CLI 内部处理（已是 WARN 级），server 透传状态 |
| 客户端 WS 异常断开 | `websockets` 异常 / 进程关闭 | server 捕获，client_count-1；最后一个退出时停子进程 |
| 客户端断网/刷新 | WebSocket close frame | 同上，浏览器侧 `app.js` 自动重连 |
| 周期修改时子进程未及时退出 | `dumper.stop()` 超时 2s | 强制 `Popen.kill()`，再重启 |
| B1 帧块乱序到达 | `frame.parse()` 检测 `offset` | 按 `offset` 排序合并（防御性，正常不会出现）|
| payload 长度不符 4800 | `assert` 失败 | server 跳过这帧，记 `[WARN]`，继续收下一帧 |
| 多个客户端改周期冲突 | 后到的覆盖 | 接受最后写者语义，加 WARN 日志 |
| 首次启动 mklink 未安装 | `subprocess` 启动失败 | 启动时 `import mklink` 探测，失败则打印清晰错误并退出 |

### 日志策略

- server 启动 / 停子进程 → `print` 到 stderr（开发期够用）
- 状态转换 → JSON 推到客户端 status bar
- 解析错误 → `[WARN]` 到 stderr，不打断
- **不**写日志文件（YAGNI）

## Core Algorithms

### Frame 解析（`frame.py`）

```python
@dataclass
class Frame:
    ts_us: int          # 设备时间戳
    payload: bytes      # 4800 字节

def parse_dump_memory_json(line: str) -> Frame:
    obj = json.loads(line)
    buf = bytearray()
    for blk in sorted(obj["blocks"], key=lambda b: b["offset"]):
        buf.extend(base64.b64decode(blk["payload_b64"]))
    assert len(buf) == FB_SIZE, f"unexpected size {len(buf)}"
    return Frame(ts_us=obj["ts_us"], payload=bytes(buf))
```

### Dumper 生命周期（`dumper.py`）

```python
class Dumper:
    def __init__(self):
        self.proc: subprocess.Popen | None = None
        self.latest_frame: Frame | None = None
        self._lock = asyncio.Lock()

    async def start(self, period_ms: int):
        if self.proc and self.proc.poll() is None:
            await self.set_period(period_ms)  # 已运行则改周期
            return
        cmd = [sys.executable, "-m", "mklink", "dump-memory",
               f"{FB_ADDR}:{FB_SIZE}",
               "--period", str(period_ms / 1000),
               "--frames", "0", "--duration", "0",
               "--json"]
        self.proc = subprocess.Popen(cmd, stdout=PIPE, text=True, bufsize=1)
        asyncio.create_task(self._reader())

    async def _reader(self):
        for line in self.proc.stdout:
            try:
                frame = parse_dump_memory_json(line)
                self.latest_frame = frame
            except Exception as e:
                log(f"[WARN] parse error: {e}")
        if self.proc.returncode != 0:
            log(f"[ERROR] dump-memory exit code {self.proc.returncode}")

    async def stop(self):
        if not self.proc: return
        try:
            self.proc.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            self.proc.wait()
        self.proc = None
        self.latest_frame = None
```

## Frontend Sketch (tools/lcd_viewer/web/)

```
┌────────────────────────────────────────────────────────────┐
│ GD32 LCD · 20 fps · COM4 · 已连接 ●          [顶栏状态]    │
├────────────────────────────────────────────────────────────┤
│                                                            │
│                                                            │
│           ┌──────────────────────────────┐                 │
│           │                              │                 │
│           │      240×160 <canvas>         │                 │
│           │   (image-rendering: pixelated)│                 │
│           │                              │                 │
│           └──────────────────────────────┘                 │
│                                                            │
│                                                            │
├────────────────────────────────────────────────────────────┤
│  [⏸ 暂停]  周期 [50] ms   [💾 保存当前帧]   [底栏控件]    │
└────────────────────────────────────────────────────────────┘
```

- 像素颜色：`bit=1` → `#50ff78`（亮绿）；`bit=0` → `#000`（黑）
- Canvas 真实尺寸 240×160，浏览器原生像素，无缩放
- 顶栏每 1s 更新 FPS / 累计帧 / 状态点
- 控件直接对应 WS JSON 控制消息

## Testing Strategy

### 单元测试（`tests/test_lcd_viewer/`，pytest）

| 模块 | 测试点 | 不依赖硬件 |
|------|--------|------------|
| `frame.py::parse_dump_memory_json` | • 正常 B1 3 块（2048+2048+704）<br>• 缺一块 → assert 失败<br>• payload_b64 解码后长度不符 → assert 失败<br>• 块乱序 → 按 offset 排序仍 OK | ✅ |
| `dumper.py::Dumper` | • 启停生命周期（mock subprocess）<br>• `_reader` 解析异常行不崩<br>• `latest_frame` 锁正确性<br>• 子进程超时强杀 | ✅（用 `subprocess.Popen` 替身）|
| `ws_server.py` | • 客户端计数 0→1→2→1→0<br>• 广播同一 `latest_frame` 引用给多客户端<br>• 客户端断连 → cleanup | ✅（用 `websockets.test`）|

### 集成测试

| 场景 | 验证 |
|------|------|
| mock `dump-memory` 输出 → server 解析 → WS 广播 | 端到端帧内容字节相等 |
| `python -m mklink dump-memory ...` 真跑（需要 MCU 在线）| 1 帧 → 1 WS 帧，4800 字节不丢 |

集成测试**默认 skip**，需 `MKVIEW_HW=1` 环境变量才执行。

### 手动验证清单（提交前必做）

```bash
# 1. 单元测试
pytest tests/test_lcd_viewer/ -v

# 2. 启动 server
python tools/lcd_viewer/server.py

# 3. 浏览器开 http://127.0.0.1:8765
#    - 看到首帧（hero pattern 测试模式）
#    - FPS 显示接近 20
#    - 改周期 50→100→50，UI 实时响应
#    - 点 💾 下载 PNG
#    - 点 ⏸ 暂停，FPS 仍跑但 canvas 不变
#    - 关闭浏览器：server 日志打印 "subprocess terminated"
#    - 再开浏览器：自动重连，首帧恢复

# 4. MCU 拔线测试
#    - 状态条变 "● error: device disconnected"
#    - 重新插线 + 关闭再开浏览器 → 自动恢复
```

### 验证命令（接受变更前必跑）

- **Gate**: `pytest tests/test_lcd_viewer/ -v`
- **Gate**: `python tools/lcd_viewer/server.py` 启动后冒烟 `curl http://127.0.0.1:8765/` 返回 200
- **手动**: 浏览器渲染验证（不可自动化）

### 测试覆盖目标

- 单元测试：核心逻辑（frame 解析、dumper 生命周期、ws 广播）≥ 80% 行覆盖
- 不写：UI 像素级快照测试（YAGNI，手动验证即可）

## Out of Scope (deferred)

- 写操作注入（按键、清屏）→ 后续若需要单独 spec
- 多区域 dump（同时看 framebuffer + 寄存器）→ 后续若需要再扩展 `dumper` 接受多 region
- LAN 访问 → 启动时加 `--host 0.0.0.0`
- 录像 / 回放 → 后续若需要
- 与 mcp_server.py 集成（共享 dump_framebuffer）→ 架构不同，本工具独立

## Open Questions

无。已通过 brainstorming session 与用户对齐所有架构决策。
