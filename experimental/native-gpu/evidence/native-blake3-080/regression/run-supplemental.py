#!/usr/bin/env python3
"""Exercise shader and command variants on the physical N100 candidate."""
import json
from pathlib import Path
import re
import subprocess

BUNDLE = "/boot/home/native-blake3-080"
LIBRARY_PATH = (BUNDLE + ":/boot/home/native-rcs-large-039/vulkan-deps:"
                "/boot/home/config/lib:/boot/system/lib")
CASES = (
    (32, "half", 4, "none"),
    (32, "green75", 4, "none"),
    (32, "green90", 4, "none"),
    (32, "original", 4, "named"),
    (32, "original", 2, "batch"),
    (32, "original", 2, "dual"),
    (32, "original", 2, "multipass"),
    (32, "original", 4, "coloronly"),
    (64, "original", 4, "coloronly"),
)

saved = []
sequences = set()
for size, profile, repeats, variant in CASES:
    setting = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    suffix = "" if variant == "none" else " " + variant
    command = (f"LIBRARY_PATH={LIBRARY_PATH} {setting}"
               f"RENKU_VK_DIRECT_TARGET=1 RENKU_VK_TRACE=1 "
               f"{BUNDLE}/triangle_probe 0 {profile} {repeats}{suffix}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    label = (f"CASE {size} profile={profile} repeats={repeats} "
             f"variant={variant} rc={result.returncode}")
    saved.extend((label, result.stdout.strip(), result.stderr.strip()))
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith('{') and '"event":"vk_queue_submit"' in line]
    match = re.search(r"hash=([0-9a-f]{8}) draws=(\d+)", result.stdout)
    assert result.returncode == 0 and match and int(match[2]) == repeats
    assert len(events) == repeats, (label, result.stdout, result.stderr)
    for index, event in enumerate(events):
        assert event["version"] == 15 and event["direct_target"] == 1
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["write_ioctl_us"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in sequences
        assert event["checked"] == event["matched"] > 0
        expected = ("28616d2d" if index == 0 and
                    variant in ("dual", "multipass") else match[1])
        assert event["pixel_hash"] == expected
        sequences.add(event["sequence"])
    print(label, "PASS", flush=True)

Path(__file__).with_name("supplemental-matrix.log").write_text(
    "\n".join(saved) + "\n")
print(f"N100 GL-extension prune: PASS ({len(CASES)} cases, {len(sequences)} draws)")
