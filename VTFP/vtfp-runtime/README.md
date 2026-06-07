# VTFP v1 Runtime (`vtfp-runtime`)

Version: see [`CHANGELOG.md`](CHANGELOG.md). Public API: [`include/vtfp/vtfp.h`](include/vtfp/vtfp.h).

This is **M2** of the VTFP范本. It provides the C state machine that runs on the MCU, polls the shared RAM header, and dispatches user-registered command handlers.

## Layers

```
┌─────────────────────────────────────────────┐
│  User firmware (your application code)    │
│  - implements handlers matching            │
│    vtfp_handler_fn signature                │
│  - calls vtfp_init + vtfp_register_handler  │
│  - calls vtfp_poll() in main loop          │
└─────────────────────────────────────────────┘
                  ↓ uses
┌─────────────────────────────────────────────┐
│  vtfp-runtime (this directory)             │
│  - vtfp_core.c: poll + dispatch + ARM      │
│  - vtfp_dispatch.c: command case table    │
│  - vtfp_arm.c: ARM state machine           │
│  - vtfp_checksum.c: CRC-32 over header    │
│  - ports/: RT-Thread / FreeRTOS / bare     │
└─────────────────────────────────────────────┘
                  ↓ depends on
┌─────────────────────────────────────────────┐
│  vtfp-protocol (M1)                        │
│  - vtfp_header.h, vtfp_command.h, etc.     │
│  - Pure declarations, no implementation   │
└─────────────────────────────────────────────┘
```

## API (one-screen reference)

```c
#include <vtfp/vtfp.h>

vtfp_config_t cfg = { ... defaults ... };
vtfp_init(&cfg);

vtfp_register_handler(0x01, my_handler, VTFP_ARM_NONE);

while (1) {
    vtfp_poll();
    rt_thread_mdelay(20);  // your RTOS sleep
}
```

## Build

The runtime is built as a static library and linked with user firmware. CMake / Make / Keil / IAR support coming in M2-T7.

## Tests

```bash
cd tests
make test
```

## Versioning

`vtfp-runtime/CHANGELOG.md` follows SemVer. Bump MAJOR for any API or wire-format-incompatible change.
