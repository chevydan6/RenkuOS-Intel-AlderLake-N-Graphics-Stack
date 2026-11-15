#!/usr/bin/env python3
"""Verify one six-vertex Vulkan draw and its existing two-draw equivalent."""
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-draw6-086"
LIBRARIES = (BUNDLE + ":/boot/home/native-blake3-080:/boot/home/native-ubo-076:"
             "/boot/home/config/lib:/boot/system/lib")
EXPECTED = {32: ("d23b1ba1", [20], 585),
            64: ("bb1f873b", [20], 3016)}


def run(size, mode, batch):
    suffix = "" if batch else "RENKU_VK_NATIVE_BATCH=0 "
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}{suffix}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={BUNDLE}/capture.bin "
               f"timeout 20 {BUNDLE}/build/triangle_probe 0 original 2 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    expected_hash, _, checked = EXPECTED[size]
    versions = [20] if batch else [15, 19]
    assert result.returncode == 0 and len(events) == len(versions), result
    assert f"hash={expected_hash}" in result.stdout, result.stdout
    assert ("calls=1" if mode == "draw6" else "draws=2") in result.stdout
    assert [event["version"] for event in events] == versions
    assert [event["gpu_draws"] for event in events] == (
        [2] if batch else [1, 1])
    assert all(event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
               and event["sequence"] == event["observed"] > 0
               and event["matched"] == event["checked"]
               for event in events)
    assert events[-1]["pixel_hash"] == expected_hash
    if batch:
        assert events[-1]["checked"] == checked
    output = HERE / f"{mode}-{size}-{'batch' if batch else 'fallback'}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{BUNDLE}/capture.bin",
                    str(output)], check=True, timeout=20)
    baseline = HERE.parent / "native-gpubatch64-084" / f"batch{size}.bin"
    assert output.read_bytes() == baseline.read_bytes(), output
    return (f"size={size} mode={mode} batch={int(batch)} rc={result.returncode}\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}")


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for mode, batch in (("draw6", True), ("draw6", False),
                            ("batchgpu", True)):
            lines.append(run(size, mode, batch))
            print(f"N100 six-vertex draw: PASS size={size} mode={mode} "
                  f"batch={int(batch)}", flush=True)
    (HERE / "hardware.log").write_text("\n".join(lines) + "\n")
