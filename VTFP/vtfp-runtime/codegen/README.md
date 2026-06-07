# VTFP Codegen

`gen_c_dispatch.py` reads a project `vtfp.yaml` and emits a C source file with stub handlers and a registration function.

## Usage

```bash
python gen_c_dispatch.py vtfp.yaml vtfp_user_dispatch.c
```

The output file is `vtfp_user_dispatch.c` with one stub per command in `vtfp.yaml`. Edit the stub bodies to implement your handlers, then call `vtfp_user_handlers_init()` from your main() after `vtfp_init()`.

## Example

Given `vtfp.yaml`:
```yaml
commands:
  - id: 1
    name: QUERY_INFO
    description: Get device info
    direction: pc_to_mcu
    requires_arm: false
  - id: 2
    name: TOGGLE_OUTPUT
    direction: pc_to_mcu
    requires_arm: true
```

`gen_c_dispatch.py` produces a file with:
- `my_query_info_handler()` — stub returning VTFP_R_NOT_INIT
- `my_toggle_output_handler()` — stub returning VTFP_R_NOT_INIT
- `vtfp_user_handlers_init()` — registers both with appropriate ARM policy

## Re-running

When you edit `vtfp.yaml` (e.g., add a new command), re-run codegen. The output file is overwritten; the markers `=== BEGIN/END user handler implementations ===` tell you where to edit.

## Validation

Before running codegen, validate `vtfp.yaml` against `vtfp.schema.json`:

```bash
jsonschema -i vtfp.yaml vtfp.schema.json
```

(codegen does not validate; it assumes the file is valid.)
