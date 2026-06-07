# VTFP v1 — Virtual Test Fixture Protocol

Version: see [`VERSION`](VERSION). Spec: [`SPEC.md`](SPEC.md).

This directory is **M1** of the VTFP范本. It contains the protocol definition only:
- C header type definitions (zero-dependency, host-pc compilable)
- Python protocol primitives (`vtfp_proto` package)
- JSON Schema for project-level `vtfp.yaml`
- Cross-language test proving C and Python agree on the wire format

The C runtime (port layer + state machine) and Python SDK (high-level agent) live in
M2 and M3 respectively, and depend on this layer.

## Build & Test

```bash
# C tests
cd c-tests && make test

# Python tests
cd py && pip install -e .[test] && pytest
```

## Versioning

`VERSION` follows [SemVer](https://semver.org/). Bump MAJOR for any wire-format
incompatibility, MINOR for backward-compatible additions, PATCH for typo/clarity.
CHANGELOG records every version.
