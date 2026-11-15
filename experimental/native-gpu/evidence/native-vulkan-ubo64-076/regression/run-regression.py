#!/usr/bin/env python3
"""Run the native GL-header compiler candidate on the physical N100."""
import json
from pathlib import Path
import re
import subprocess

BUNDLE = "/boot/home/native-ubo-076"
LIBRARY_PATH = (BUNDLE + ":/boot/home/native-rcs-large-039/vulkan-deps:"
                "/boot/home/config/lib:/boot/system/lib")
cases = []
for size in (32, 64):
    for profile in ("original", "rgb"):
        for shape in range(4):
            cases.append((size, shape, profile, 2, "none"))
    for variant in ("rotate", "depth", "offset", "firstvertex",
                    "pageoffset", "rotateoffset", "rotatefirst",
                    "rotatepage", "depthoffset"):
        cases.append((size, 0, "original", 4, variant))
for profile in ("fragcoord32", "fragcoord", "fragcoord128"):
    cases.append((64, 0, profile, 4, "none"))
for variant in ("offset", "pageoffset"):
    cases.append((64, 0, "fragcoord128", 4, variant))

saved = []
sequences = set()
for size, shape, profile, repeats, variant in cases:
    setting = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    suffix = "" if variant == "none" else " " + variant
    command = (f"LIBRARY_PATH={LIBRARY_PATH} {setting}"
               f"RENKU_VK_DIRECT_TARGET=1 RENKU_VK_TRACE=1 "
               f"{BUNDLE}/triangle_probe {shape} {profile} {repeats}{suffix}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    label = (f"CASE {size} shape={shape} profile={profile} "
             f"repeats={repeats} variant={variant} rc={result.returncode}")
    saved.extend((label, result.stdout.strip(), result.stderr.strip()))
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith('{') and '"event":"vk_queue_submit"' in line]
    match = re.search(r"triangle: PASS shape=\d hash=([0-9a-f]{8}) "
                      r"draws=(\d+)", result.stdout)
    assert result.returncode == 0 and match and int(match[2]) == repeats
    assert len(events) == repeats, (label, result.stdout, result.stderr)
    for event in events:
        assert event["version"] == 15 and event["direct_target"] == 1
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["write_ioctl_us"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in sequences
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == match[1]
        sequences.add(event["sequence"])
    print(label, "PASS", flush=True)

path = Path(__file__).with_name("physical-matrix.log")
path.write_text("\n".join(saved) + "\n")
print(f"N100 native GL-header compiler: PASS ({len(cases)} cases, {len(sequences)} draws)")
