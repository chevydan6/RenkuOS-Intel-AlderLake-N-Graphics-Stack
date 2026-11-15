#!/usr/bin/env python3
"""Verify nonzero Vulkan UBO descriptor offsets on the physical N100."""
import json
from pathlib import Path
import subprocess

bundle = "/boot/home/native-ubo-077"
lines = []
for size, expected in ((32, "4979062d"), (64, "8df12ff5")):
    command = (
        f"LIBRARY_PATH={bundle}:/boot/home/native-ubo-076:"
        "/boot/home/config/lib:/boot/system/lib "
        f"RENKU_VK_TRACE=1 {'RENKU_VK_TARGET_64=1 ' if size == 64 else ''}"
        f"timeout 20 {bundle}/triangle_probe 0 ubooffset 2"
    )
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    assert result.returncode == 0 and len(events) == 2, (size, result)
    assert f"hash={expected} draws=2" in result.stdout, (size, result.stdout)
    for event in events:
        assert event["version"] == 17 and event["uniform_offset"] == 16
        assert event["sequence"] == event["observed"] > 0
        assert event["matched"] == event["checked"] == (3476 if size == 64 else 777)
        assert event["fault"] == event["csb_error"] == event["ioctl"] == 0
    lines.extend((f"size={size} rc={result.returncode}", result.stdout.strip(),
                  result.stderr.strip()))
    print(f"N100 UBO descriptor offset: PASS size={size} draws=2", flush=True)
Path(__file__).with_name("offset-hardware.log").write_text("\n".join(lines) + "\n")
