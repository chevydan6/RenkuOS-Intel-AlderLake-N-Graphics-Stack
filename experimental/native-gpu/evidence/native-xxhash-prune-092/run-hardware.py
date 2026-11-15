#!/usr/bin/env python3
"""Verify cross-pass LOAD against same-pass rendering on the physical N100."""
import hashlib
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-loadpass-091"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
EXPECTED = {32: ("225b1ba1", "893f4100ba027396244478779150c0c1e44aad4f68d2cf35cfe3f7f01d00077d"),
            64: ("bb1f873b", "b981f708c614a79c255941d9ed7f9f31d30653875c48e8db761c1d673093c575")}


def run(size, mode, direct=True):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    fallback = "" if direct else "RENKU_VK_DIRECT_TARGET=0 "
    capture = f"{BUNDLE}/capture-{size}-{mode}.bin"
    command = (f"LIBRARY_PATH={LIBRARIES} {target}{fallback}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={capture} timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 2 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    if not direct:
        assert result.returncode == 26 and not events, result
        return f"mode={mode} size={size} direct=0 rc=26\n{result.stderr.strip()}"
    assert result.returncode == 0 and len(events) == 2, result
    expected_versions = [15, 15] if mode == "multipass" else [15, 19]
    assert [e["version"] for e in events] == expected_versions, events
    assert all(e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
               and e["sequence"] == e["observed"] > 0
               and e["checked"] == e["matched"] > 0 for e in events), events
    if mode != "multipass":
        assert events[-1]["pixel_hash"] == EXPECTED[size][0], events
    output = HERE / f"capture-{size}-{mode}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    if mode != "multipass":
        assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][1]
    return (f"mode={mode} size={size} direct=1 rc=0\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}")


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for mode in ("loadpass", "multidraw"):
            lines.append(run(size, mode))
            print(f"N100 {size} {mode}: PASS", flush=True)
        assert (HERE / f"capture-{size}-loadpass.bin").read_bytes() == (
            HERE / f"capture-{size}-multidraw.bin").read_bytes()
    lines.append(run(32, "multipass"))
    assert (HERE / "capture-32-multipass.bin").read_bytes() != (
        HERE / "capture-32-loadpass.bin").read_bytes()
    print("N100 DONT_CARE multipass: PASS", flush=True)
    lines.append(run(32, "loadpass", direct=False))
    print("N100 LOAD without direct target: rejected before submit", flush=True)
    (HERE / "hardware.log").write_text("\n".join(lines) + "\n")
