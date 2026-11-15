#!/usr/bin/env python3
"""Run three green-gradient scales and the fallback path on the N100."""
import json
from pathlib import Path
import re
import subprocess

BUNDLE = "/boot/home/native-ubo-079"
LIBRARY_PATH = (BUNDLE + ":/boot/home/native-rcs-large-039/vulkan-deps:"
                "/boot/home/config/lib:/boot/system/lib")
CASES = (("fragcoordgreen32", True, "6e59daf5"),
         ("fragcoordgreen", True, "f25bcaf5"),
         ("fragcoordgreen128", True, "649956f5"),
         ("fragcoordgreen", False, "f25bcaf5"))
lines = []
sequences = set()
for profile, direct, expected_hash in CASES:
    command = (f"LIBRARY_PATH={LIBRARY_PATH} RENKU_VK_TARGET_64=1 "
               f"RENKU_VK_DIRECT_TARGET={int(direct)} RENKU_VK_TRACE=1 "
               f"{BUNDLE}/triangle_probe 0 {profile} 4")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    label = f"CASE profile={profile} direct={int(direct)} rc={result.returncode}"
    lines.extend((label, result.stdout.strip(), result.stderr.strip()))
    assert result.returncode == 0
    assert re.search(rf"hash={expected_hash} draws=4", result.stdout)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith('{') and '"event":"vk_queue_submit"' in line]
    assert len(events) == 4
    for event in events:
        assert event["version"] == (15 if direct else 12)
        assert event["direct_target"] == int(direct)
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in sequences
        assert event["checked"] == event["matched"] == 3476
        assert event["pixel_hash"] == expected_hash
        if direct:
            assert event["write_ioctl_us"] == 0
        sequences.add(event["sequence"])
    print(label, "PASS", flush=True)
Path(__file__).with_name("green-matrix.log").write_text("\n".join(lines) + "\n")
print(f"Green gradient N100: PASS ({len(CASES)} cases, {len(sequences)} draws)")
