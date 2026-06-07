# Changelog

## 0.1.0 — 2026-06-05
- Initial VTFP v1 protocol layer
- `vtfp_header_t`: 44-byte packed struct (magic + version + features + 9 fields)
- Command ID partition map: 0x01-0x0F user, 0x10-0x1F standard, 0xF0-0xFF safety
- Standard commands: QUERY_INFO (0x10), QUERY_STATE (0x11), RESET (0x12), HEARTBEAT (0x13)
- Safety: ARM (0xFE), DISARM (0xFF)
- Result codes 0..6, 0xFF
- CRC-32 checksum over header bytes 0..35 (excluding checksum+reserved)
- JSON Schema v0 for project-level `vtfp.yaml`
