#!/usr/bin/env bash
# Run all C tests in order. Exits non-zero on first failure.
set -e
cd "$(dirname "$0")"
make clean
make test
echo ""
echo "=== M1 C side: ALL PASSED ==="
