# Scaffolding Workflow (`init-vtfp.py`)

The `init-vtfp.py` tool is the entry point for adding VTFP support to a new project.

## 5 stages

```
init-vtfp.py discover     → init-vtfp.py interview   → init-vtfp.py codegen   → init-vtfp.py verify
       ↓                            ↓                          ↓                          ↓
  scan project,             Q&A with user,             generate C + Python     run protocol tests,
  detect RTOS,              write vtfp.yaml           + MCP server from       no hardware needed
  propose defaults                                     vtfp.yaml
```

## Stage 1: Discover

Scan an existing project to detect RTOS, toolchain, and existing MKLink usage.

```bash
$ python tools/init-vtfp.py discover --root .

project_root: D:\Projects\GEC6100D
rtos: rt-thread
toolchain: keil
has_mklink_resource: true
has_test_fixture: true
rtos_version: ''
main_file: ''
warnings: []
recommendations: {}
```

Output is YAML — pipe to a file if you want to use it as input to Stage 2.

## Stage 2: Interview

Interactive Q&A that writes `vtfp.yaml`. Defaults are derived from the Discover output (when run after Discover).

```bash
$ python tools/init-vtfp.py interview --root .

=== VTFP v1 Project Setup Interview ===
Press Enter to accept defaults.

Project name [my-controller]: my-plc
Product ID (e.g. MC-1.0) [MC-1.0]: PLC-1.0
Model code (uint16) [0]: 
RTOS type (rt-thread / freertos / bare) [rt-thread]: 
...

Wrote vtfp.yaml
```

If you need to come back later, use `--resume` to pick up where you left off (state is in `.vtfp-state/{project_id}/interview.json`).

## Stage 3: Codegen

Read `vtfp.yaml` and write the runtime code. Without `--diff`, this overwrites generated files in `--out-dir` (default: cwd).

```bash
$ python tools/init-vtfp.py codegen --config vtfp.yaml --out-dir .

Generated: vtfp_user_dispatch.c   # stub handlers — replace with your impl
```

With `--diff`, prints unified diffs instead of writing. Useful for CI checks or previews.

## Stage 4: Verify

Run the protocol-level test suite (no hardware). Validates `vtfp.yaml` against the schema and runs all M1 + M3 pytest suites.

```bash
$ python tools/init-vtfp.py verify --config vtfp.yaml
OK: vtfp.yaml validates against vtfp.schema.json
--- Running vtfp_proto tests ---
OK: vtfp_proto tests passed
--- Running mklink_vtfp tests ---
OK: mklink_vtfp tests passed
```

Exits 0 on success, non-zero on failure.

## State persistence

The Interview subcommand saves intermediate state to `.vtfp-state/{project_id}/interview.json`. The state is overwritten each time you save. To start fresh, delete the directory.

## Common pitfalls

- **Don't forget to commit `vtfp.yaml`.** It IS the source of truth for your project — not the generated C/SDK code.
- **Run `verify` after every `vtfp.yaml` change.** A wrong schema or missing required field will silently fail at codegen time.
- **Use `--diff` in CI** to catch unintended changes to generated files.
