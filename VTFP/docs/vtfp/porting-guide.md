# Porting Guide

This document explains how to add a new RTOS port or a new domain to VTFP v1.

## Adding a new RTOS port

Suppose you want to add support for `MyRTOS` (a hypothetical new RTOS).

### Step 1: Add the port file

Create `vtfp-runtime/src/ports/vtfp_port_myrtos.c`:

```c
#include "vtfp_port.h"

#ifdef MYRTOS_CONFIG_H  /* detect MyRTOS via a config header */

#include "myrtos.h"

uint32_t vtfp_port_now_ms(void) {
    return myrtos_tick_get_ms();
}

void vtfp_port_sleep_ms(uint32_t ms) {
    myrtos_sleep_ms(ms);
}

void vtfp_port_init(void) { /* optional */ }

#else  /* host-pc build fallback */

__attribute__((weak)) uint32_t vtfp_port_now_ms(void) {
    static uint32_t fake = 0;
    return fake++;
}

void vtfp_port_sleep_ms(uint32_t ms) { (void)ms; }
void vtfp_port_init(void) {}

#endif
```

### Step 2: Add a `case` in the Makefile

In `vtfp-runtime/tests/Makefile` (or your build system), allow building the new port:

```makefile
# Add a build option
MYRTOS_PORT ?= ../src/ports/vtfp_port_myrtos.c
```

### Step 3: Add to `vtfp.yaml` schema

The `rtos.type` enum in `vtfp-protocol/vtfp.schema.json` needs to include `"myrtos"`:

```json
"rtos": {
  "type": { "enum": ["rt-thread", "freertos", "bare", "myrtos"] }
}
```

### Step 4: Add a sample

Create `samples/abstract-controller/myrtos/main.c` (mirror the FreeRTOS or RT-Thread sample) plus a `vtfp.yaml` with `rtos.type: myrtos`.

### Step 5: Add a port layer test

In `vtfp-runtime/tests/`, add a `test_port_myrtos.c` that mocks MyRTOS APIs and verifies `vtfp_port_now_ms()` returns the right value.

## Adding a new domain (replacing genset/PLC/inverter semantics)

VTFP is domain-neutral. The shared RAM is just a command/response protocol — what the commands MEAN is up to your project.

### Step 1: Write your `vtfp.yaml`

Define your commands. The `id` is a user-command ID (0x01..0x0F), the `name` is UPPER_SNAKE_CASE, the `requires_arm` flag gates dangerous operations:

```yaml
commands:
  - id: 1
    name: SET_SPEED
    description: Set motor speed (RPM)
    direction: pc_to_mcu
    requires_arm: true   # dangerous — could overspeed
    request_schema:
      type: int
      size_bytes: 4
  - id: 2
    name: READ_TEMPERATURE
    direction: pc_to_mcu
    requires_arm: false
```

### Step 2: Run `init-vtfp.py codegen`

This generates:
- `vtfp_user_dispatch.c` — stub handlers (replace with your impl)
- `mcp_server.py` — FastMCP server exposing your commands as tools

### Step 3: Implement the handlers

Edit `vtfp_user_dispatch.c` and replace each stub body with your real implementation. The function signature is:

```c
static int32_t my_set_speed_handler(const vtfp_request_t *req, vtfp_response_t *resp) {
    uint32_t rpm = req->param;
    if (rpm > MAX_RPM) return (int32_t)VTFP_R_PARAM_RANGE;
    motor_set_speed(rpm);
    return 0;
}
```

### Step 4: Connect to your existing application

Call `vtfp_poll()` in your main loop (typically every 20ms). The runtime handles the rest (checksum, ARM, dispatch).

## Versioning rules

The protocol is **forward-compatible within MAJOR version**:
- v0.1.x → v0.2.x: same wire format, new commands may be added
- v0.x.y → v1.0.0: protocol change (breaking)

The Python SDK and C runtime follow the protocol version. If you bump MAJOR, you must update both sides.
