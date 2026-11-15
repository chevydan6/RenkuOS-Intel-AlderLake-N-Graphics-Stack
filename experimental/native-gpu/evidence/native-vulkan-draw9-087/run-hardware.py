#!/usr/bin/env python3
"""Compare one nine-vertex draw with three established triangle draws on N100."""
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-draw9-087"
LIBRARIES = (BUNDLE + ":/boot/home/native-blake3-080:/boot/home/native-ubo-076:"
             "/boot/home/config/lib:/boot/system/lib")
EXPECTED = {32: {"three": "cdad4a41", "page": "d23b1ba1"},
            64: {"three": "9bd44344", "page": "bb1f873b"}}


def run(size, mode):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    count = 2 if mode == "drawpage" else 3
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={BUNDLE}/capture.bin "
               f"timeout 20 {BUNDLE}/triangle_probe 0 original {count} {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    expected_hash = EXPECTED[size]["page" if mode == "drawpage" else "three"]
    versions = [15, 19] if mode == "drawpage" else [20, 19]
    offsets = [4096 - 48, 4096] if mode == "drawpage" else [0, 96]
    assert result.returncode == 0 and len(events) == 2, result
    assert f"hash={expected_hash}" in result.stdout, result.stdout
    assert f"calls={3 if mode == 'draw3x3' else 1}" in result.stdout
    assert [event["version"] for event in events] == versions
    assert [event["vertex_offset"] for event in events] == offsets
    assert [event["gpu_draws"] for event in events] == (
        [1, 1] if mode == "drawpage" else [2, 1])
    assert all(event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
               and event["sequence"] == event["observed"] > 0
               and event["matched"] == event["checked"] > 0
               for event in events)
    assert events[-1]["pixel_hash"] == expected_hash
    output = HERE / f"{mode}-{size}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{BUNDLE}/capture.bin",
                    str(output)], check=True, timeout=20)
    if mode == "draw9":
        assert output.read_bytes() == (HERE / f"draw3x3-{size}.bin").read_bytes()
    if mode == "drawpage":
        baseline = HERE.parent / "native-gpubatch64-084" / f"batch{size}.bin"
        assert output.read_bytes() == baseline.read_bytes()
    return (f"size={size} mode={mode} rc={result.returncode}\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}")


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for mode in ("draw3x3", "draw9", "drawpage"):
            lines.append(run(size, mode))
            print(f"N100 multi-primitive draw: PASS size={size} mode={mode}",
                  flush=True)
    (HERE / "hardware.log").write_text("\n".join(lines) + "\n")
