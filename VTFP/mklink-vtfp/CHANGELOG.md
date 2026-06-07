# Changelog

## 0.1.0 — 2026-06-05
- Initial mklink-vtfp (M3)
- Transport layer: MKLinkSerialBridge (pyserial wrapper) + dump_memory stub
- Port config: env-var-driven (MKLINK_PORT, MKLINK_BAUD)
- VTFPAgent high-level SDK: discover, query_info, query_state, arm, disarm, invoke
- Exception hierarchy: VTFPAgentError, VTFPAgentNotFoundError, VTFPAgentVersionError, VTFPAgentCorruptError, VTFPAgentTimeoutError, VTFPAgentSafetyError, VTFPAgentDataError, VTFPAgentUnknownCmdError
- Result types: QueryInfoResult, CommandResult
- Codegen: generate_mcp_server (FastMCP) + generate_c_dispatch (delegates to vtfp-runtime)
- 19 Python tests pass (transport, codegen, sdk with mock bridge)
- M1+M2 still pass: 328 M1 Python + 4 M2 C test binaries
