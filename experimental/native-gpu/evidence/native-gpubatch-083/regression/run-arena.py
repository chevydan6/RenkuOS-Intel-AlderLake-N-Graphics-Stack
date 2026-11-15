#!/usr/bin/env python3
"""Verify ordinary fallback and high-VA draws on the physical N100."""
import json
from pathlib import Path
import re
import subprocess

BUNDLE = "/boot/home/native-gpubatch-083"
LIBS = BUNDLE + ":/boot/home/config/lib:/boot/system/lib"
root = Path(__file__).parent


def run(size, direct, highva):
    env = (f"LIBRARY_PATH={LIBS} RENKU_VK_TRACE=1 "
           f"RENKU_VK_DIRECT_TARGET={int(direct)}")
    if size == 64:
        env += " RENKU_VK_TARGET_64=1"
    variant = " highva" if highva else ""
    command = f"{env} {BUNDLE}/triangle_probe 0 original 4{variant}"
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    label = (f"CASE {size} shape=0 profile=original repeats=4 "
             f"direct={int(direct)} highva={int(highva)} rc={result.returncode}")
    lines = [label, result.stdout.strip(), result.stderr.strip()]
    assert result.returncode == 0, (label, result.stdout, result.stderr)
    expected = "28616d2d" if size == 32 else "8aaee2f5"
    assert re.search(rf"hash={expected} draws=4", result.stdout), lines
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith('{')]
    assert len(events) == 4, lines
    for event in events:
        assert event["event"] == "vk_queue_submit"
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == expected
        assert event["direct_target"] == int(direct)
        assert event["version"] == (15 if direct else 7 if size == 32 else 10)
        for field in ("vertex_gpu_va", "shader_gpu_va", "image_gpu_va"):
            address = int(event[field], 16)
            assert (address >= 0x420000 if highva else address >= 0x410000)
            assert address < 0x450000
    return lines


fallback = []
for size in (32, 64):
    fallback.extend(run(size, False, False))
(root / "fallback.log").write_text("\n".join(fallback) + "\n")

highva = []
for direct in (False, True):
    for size in (32, 64):
        highva.extend(run(size, direct, True))
(root / "highva-matrix.log").write_text("\n".join(highva) + "\n")
print("N100 resource arena: PASS (6 cases, 24 draws, 16 high-VA draws)")
