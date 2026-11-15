#!/usr/bin/env python3
"""Check vertex and UBO memory binding offsets on the physical N100."""
import json
from pathlib import Path
import subprocess

bundle = "/boot/home/native-ubo-078"
lines = []
for size in (32, 64):
    for profile, variant, version, vertex_offset, uniform_offset, direct in (
        ("original", "memoryoffset", 15, 4096, 0, 1),
        ("original", "memoryoffset", 13, 4096, 0, 0),
        ("ubomemoffset", "", 17, 0, 4096, 1),
        ("ubooffset", "", 17, 0, 16, 1),
    ):
        command = (
            f"LIBRARY_PATH={bundle}:/boot/home/native-ubo-077:"
            f"/boot/home/config/lib:/boot/system/lib RENKU_VK_DIRECT_TARGET={direct} "
            "RENKU_VK_TRACE=1 "
            f"{'RENKU_VK_TARGET_64=1 ' if size == 64 else ''}"
            f"timeout 20 {bundle}/triangle_probe 0 {profile} 2 {variant}"
        )
        result = subprocess.run(["ssh", "renkuos-n100", command],
                                capture_output=True, text=True, timeout=30)
        events = [json.loads(line) for line in result.stderr.splitlines()
                  if line.startswith("{") and '"event":"vk_queue_submit"' in line]
        expected_hash = ("8df12ff5" if size == 64 else "4979062d") if profile != "original" else (
            "8aaee2f5" if size == 64 else "28616d2d")
        assert result.returncode == 0 and len(events) == 2, (size, profile, result)
        assert f"hash={expected_hash} draws=2" in result.stdout, result.stdout
        for event in events:
            assert (event["version"], event["vertex_offset"], event["uniform_offset"]) == (
                version, vertex_offset, uniform_offset)
            assert event["direct_target"] == direct
            assert event["sequence"] == event["observed"] > 0
            assert event["matched"] == event["checked"] == (3476 if size == 64 else 777)
            assert event["ioctl"] == event["fault"] == event["csb_error"] == 0
        lines.extend((f"size={size} profile={profile} direct={direct} rc={result.returncode}",
                      result.stdout.strip(), result.stderr.strip()))
        print(f"N100 memory offset: PASS size={size} profile={profile} "
              f"direct={direct} draws=2", flush=True)
Path(__file__).with_name("memory-offset-hardware.log").write_text("\n".join(lines) + "\n")
