# mklink-vtfp

Python SDK for VTFP v1 firmware runtime. Provides a high-level `VTFPAgent` for AI/MCP clients to control embedded MCUs over MKLink serial.

## Status

**M3 — in progress.** M3-T1 (transport layer) done. M3-T2 will add the high-level agent API.

## Install

```bash
# From PyPI (when published)
pip install mklink-vtfp

# From source (editable)
pip install -e ".[test]"
```

## Quick start (after M3-T2)

```python
from mklink_vtfp import VTFPAgent

agent = VTFPAgent(port="COM5", config="vtfp.yaml")
agent.discover()
info = agent.query_info()
print(info.product_id, info.fw_version, info.features)
result = agent.invoke("TOGGLE_OUTPUT")
```

## Components

- `mklink_vtfp.transport.bridge` — MKLinkSerialBridge (pyserial wrapper)
- `mklink_vtfp.transport.dump_memory` — high-throughput streaming protocol (stub for now)
- `mklink_vtfp.transport.port_config` — env-var-driven port config
- `mklink_vtfp.sdk` — VTFPAgent (M3-T2)
- `mklink_vtfp.codegen` — vtfp.yaml → MCP server + C dispatch (M3-T3)
