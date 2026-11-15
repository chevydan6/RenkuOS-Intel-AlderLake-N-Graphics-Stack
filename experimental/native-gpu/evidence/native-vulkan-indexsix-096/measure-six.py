#!/usr/bin/env python3
"""Compare blocking owner submission time for N100 two-triangle paths."""
import json
from pathlib import Path
import statistics
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-indexsix-096"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
VERSIONS = {"indexsix": [24], "indexpair": [22, 23], "indexpairref": [20]}


def measure(size, mode):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    repeats = 2 if mode != "indexsix" else 1
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"timeout 20 {BUNDLE}/triangle_probe 0 original {repeats} {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    assert result.returncode == 0 and [e["version"] for e in events] == VERSIONS[mode], result
    assert all(e["ioctl"] == e["fault"] == 0 and e["stage"] == 12
               and e["checked"] == e["matched"] > 0 for e in events)
    return sum(e["submit_us"] for e in events)


if __name__ == "__main__":
    results = {}
    modes = tuple(VERSIONS)
    for size in (32, 64):
        samples = {mode: [] for mode in modes}
        for mode in modes:
            for _ in range(3):
                measure(size, mode)
        for iteration in range(20):
            ordered = modes[iteration % 3:] + modes[:iteration % 3]
            for mode in ordered:
                samples[mode].append(measure(size, mode))
        results[size] = {
            "samples_us": samples,
            "median_us": {mode: statistics.median(samples[mode]) for mode in modes},
        }
        print(f"N100 {size}: " + ", ".join(
            f"{mode} {results[size]['median_us'][mode]} us"
            for mode in modes), flush=True)
    (HERE / "six-submit-times.json").write_text(json.dumps({
        "scope": "sum of vk_queue_submit submit_us per process; blocking owner ioctl and wait, not frame latency or isolated GPU time",
        "warmups_per_mode_per_size": 3,
        "runs_per_mode_per_size": 20,
        "results": results,
    }, indent=2) + "\n")
