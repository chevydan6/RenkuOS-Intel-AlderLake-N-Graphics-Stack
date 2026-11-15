#!/usr/bin/env python3
"""Verify Vulkan image memory offsets through native N100 rendering."""
import json
from pathlib import Path
import subprocess

bundle = "/boot/home/native-gpubatch-083"
lines = []
for size in (32, 64):
    for profile, variant, direct, version, uniform_offset in (
        ("original", "imageoffset", 1, 18, 0),
        ("original", "imageoffset", 0, 7 if size == 32 else 10, 0),
        ("original", "rotateimageoffset", 1, 18, 0),
        ("ubo", "imageoffset", 1, 18, 0),
        ("ubooffset", "imageoffset", 1, 18, 16),
        ("ubomemoffset", "imageoffset", 1, 18, 4096),
    ):
        command = (
            f"LIBRARY_PATH={bundle}:/boot/home/native-ubo-078:"
            f"/boot/home/config/lib:/boot/system/lib RENKU_VK_DIRECT_TARGET={direct} "
            f"RENKU_VK_TRACE=1 {'RENKU_VK_TARGET_64=1 ' if size == 64 else ''}"
            f"timeout 20 {bundle}/triangle_probe 0 {profile} 2 {variant}"
        )
        result = subprocess.run(["ssh", "renkuos-n100", command],
                                capture_output=True, text=True, timeout=30)
        events = [json.loads(line) for line in result.stderr.splitlines()
                  if line.startswith("{") and '"event":"vk_queue_submit"' in line]
        expected_hash = ("7b48f3e5" if size == 64 else "77323fd5") if variant == "rotateimageoffset" else (
            ("8df12ff5" if size == 64 else "4979062d") if profile != "original" else
            ("8aaee2f5" if size == 64 else "28616d2d"))
        assert result.returncode == 0 and len(events) == 2, (size, profile, result)
        assert f"hash={expected_hash} draws=2" in result.stdout, result.stdout
        for event in events:
            assert (event["version"], event["image_offset"], event["uniform_offset"],
                    event["direct_target"]) == (version, 4096, uniform_offset, direct)
            assert bool(event["vertex_shader_bo"]) == (variant == "rotateimageoffset")
            assert event["sequence"] == event["observed"] > 0
            assert event["matched"] == event["checked"] == (3476 if size == 64 else 777)
            assert event["ioctl"] == event["write_ioctl"] == 0
            assert event["fault"] == event["csb_error"] == 0
        lines.extend((f"size={size} profile={profile} variant={variant} "
                      f"direct={direct} rc={result.returncode}",
                      result.stdout.strip(), result.stderr.strip()))
        print(f"N100 image offset: PASS size={size} profile={profile} variant={variant} "
              f"direct={direct} draws=2", flush=True)
Path(__file__).with_name("image-offset-hardware.log").write_text("\n".join(lines) + "\n")
