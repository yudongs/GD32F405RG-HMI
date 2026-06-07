#!/usr/bin/env bash
# test_schema.sh — validate sample_vtfp.yaml against vtfp.schema.json
# Run from anywhere: ./test_schema.sh

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCHEMA="${SCRIPT_DIR}/../vtfp.schema.json"
SAMPLE="${SCRIPT_DIR}/sample_vtfp.yaml"

if ! command -v python >/dev/null 2>&1; then
    echo "python not found in PATH"
    exit 1
fi

if ! python -c "import jsonschema, yaml" >/dev/null 2>&1; then
    echo "Missing Python deps. Install with: pip install jsonschema pyyaml"
    exit 1
fi

echo "--- validating sample_vtfp.yaml against vtfp.schema.json ---"
python - "${SAMPLE}" "${SCHEMA}" <<'PY'
import sys, json
import yaml
from jsonschema import Draft7Validator

sample_path, schema_path = sys.argv[1], sys.argv[2]
with open(sample_path, "r", encoding="utf-8") as f:
    instance = yaml.safe_load(f)
with open(schema_path, "r", encoding="utf-8") as f:
    schema = json.load(f)

validator = Draft7Validator(schema)
errors = list(validator.iter_errors(instance))
if errors:
    for err in errors:
        path = "/".join(str(p) for p in err.absolute_path) or "<root>"
        print(f"ERROR: {path}: {err.message}")
    sys.exit(1)
PY
echo "OK: sample_vtfp.yaml is valid"
