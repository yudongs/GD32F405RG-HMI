"""Verify VTFP symbols in the compiled .axf"""
import subprocess

READELF = r"C:/Program Files/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi/bin/arm-none-eabi-readelf.EXE"
AXF = r"D:/xwechat_files/wxid_r61ppcpj6ncp22_b17d/msg/file/2026-06/GD32F405RG-HMI-529/GD32F405RG-HMI-529/GD32F405RG-HMI/Project/Objects/GD32F405RG.axf"

r = subprocess.run([READELF, "-sW", AXF], capture_output=True, text=True)
lines = r.stdout.splitlines()

vtfp_syms = [l for l in lines if any(k in l for k in ("vtfp_", "VTFP_", "s_vtfp", "vTaskVTFP"))]
print(f"Total VTFP-related symbols: {len(vtfp_syms)}")
print()
print("--- Handler / Runtime ---")
for s in vtfp_syms:
    if any(k in s for k in ("vtfp_init", "vtfp_poll", "vtfp_register", "vtfp_port", "vtfp_user", "vtfp_arm", "vtfp_crc", "vtfp_check")):
        parts = s.split()
        if len(parts) >= 8:
            print(f"  {parts[-1]}  size={parts[2]}  addr={parts[1]}")
print()
print("--- Section placement ---")
sections = subprocess.run([READELF, "-SW", AXF], capture_output=True, text=True).stdout
for line in sections.splitlines():
    if ".vtfp" in line.lower() or ".bss" in line or "RAM" in line.upper():
        print("  " + line)
