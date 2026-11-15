#!/usr/bin/env python3
"""Run binary semaphore ordered draws on the physical N100."""
import json
from pathlib import Path
import re
import subprocess

BUNDLE = "/boot/home/native-multidraw-082"
LIBRARY_PATH = (BUNDLE + ":/boot/home/native-rcs-large-039/vulkan-deps:"
                "/boot/home/config/lib:/boot/system/lib")
CASES = [(size, shape, "original", True)
         for size in (32, 64) for shape in range(4)]
CASES += [(32, 0, "rgb", True), (64, 0, "rgb", True),
          (64, 0, "fragcoord", True),
          (64, 0, "fragcoordgreen", True),
          (32, 0, "original", False), (64, 0, "original", False)]

lines = []
sequences = set()
for size, shape, profile, direct in CASES:
    setting = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    command = (f"LIBRARY_PATH={LIBRARY_PATH} {setting}"
               f"RENKU_VK_DIRECT_TARGET={int(direct)} RENKU_VK_TRACE=1 "
               f"{BUNDLE}/triangle_probe {shape} {profile} 2 semaphore")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    label = (f"CASE {size} shape={shape} profile={profile} "
             f"direct={int(direct)} rc={result.returncode}")
    lines.extend((label, result.stdout.strip(), result.stderr.strip()))
    match = re.search(r"hash=([0-9a-f]{8}) draws=2", result.stdout)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith('{') and '"event":"vk_queue_submit"' in line]
    assert result.returncode == 0 and match and len(events) == 2, label
    for event in events:
        assert event["direct_target"] == int(direct)
        assert event["version"] == (15 if direct else (7 if size == 32 else
                                    12 if profile.startswith("fragcoord") else 10))
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in sequences
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == match[1]
        if direct:
            assert event["write_ioctl_us"] == 0
        sequences.add(event["sequence"])
    print(label, "PASS", flush=True)

Path(__file__).with_name("semaphore-matrix.log").write_text("\n".join(lines) + "\n")
print(f"N100 binary semaphores: PASS ({len(CASES)} cases, {len(sequences)} draws)")
