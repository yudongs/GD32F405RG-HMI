# Abstract Controller — RT-Thread Sample

A minimal VTFP v1 application demonstrating end-to-end integration with RT-Thread.

## What it does

- Implements 3 user commands: `QUERY_STATE`, `SET_PARAM`, `TOGGLE_OUTPUT`
- `TOGGLE_OUTPUT` requires ARM
- Internal state: 4 params + 8-bit output + 32-bit button counter

## File layout

```
rtthread/
├── vtfp.yaml                # project config (drives codegen)
├── vtfp_user_dispatch.c     # hand-written handlers (NOT generated)
├── main.c                   # entry point: vtfp_init + register + main loop
└── README.md
```

## Build (RT-Thread + GCC)

Add to your RT-Thread project:

```bash
# 1. Install the vtfp-protocol Python package (for codegen)
pip install -e vtfp-protocol/py

# 2. Generate the C dispatch source (if you want to use the codegen)
python vtfp-runtime/codegen/gen_c_dispatch.py vtfp.yaml vtfp_user_dispatch_gen.c

# 3. Add these to your build:
#    Include paths: -Ivtfp-protocol/c-types -Ivtfp-runtime/include
#    Source files:  vtfp-runtime/src/*.c + vtfp-runtime/src/ports/vtfp_port_rtthread.c
#                   this directory: main.c + vtfp_user_dispatch.c
```

## Run

After loading the firmware, on the PC side:

```python
from mklink_vtfp import VTFPAgent
agent = VTFPAgent(port="COM5", config="vtfp.yaml")
info = agent.discover()
print(info.product_id, info.fw_version)

# Safe command
state = agent.invoke("QUERY_STATE")  # 8-byte state word
print(state.data.hex())

# Safe command
agent.invoke("SET_PARAM", param=0x00000001)  # params[0] = 1

# Dangerous command (ARM first)
agent.arm()
agent.invoke("TOGGLE_OUTPUT", param=0x00)  # toggle bit 0
# auto_disarm means subsequent dangerous commands need a fresh ARM
```

## See also

- [`../../../../vtfp-protocol/SPEC.md`](../../../../vtfp-protocol/SPEC.md) — protocol spec
- [`../../../../vtfp-runtime/`](../../../../vtfp-runtime/) — C runtime
- [`../../../../mklink-vtfp/`](../../../../mklink-vtfp/) — Python SDK
- [`../../../../tools/init-vtfp.py`](../../../../tools/init-vtfp.py) — scaffolding tool
