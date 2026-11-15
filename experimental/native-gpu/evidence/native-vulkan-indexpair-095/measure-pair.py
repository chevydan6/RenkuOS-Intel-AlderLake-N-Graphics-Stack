#!/usr/bin/env python3
"""Compare blocking native submit time for two indexed and one batched draw."""
import json
from pathlib import Path
import statistics
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-indexpair-095"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")


def measure(size, mode):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"timeout 20 {BUNDLE}/triangle_probe 0 original 2 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    expected = [22, 23] if mode == "indexpair" else [20]
    assert result.returncode == 0 and [e["version"] for e in events] == expected, result
    assert all(e["ioctl"] == e["fault"] == 0 and e["stage"] == 12
               and e["checked"] == e["matched"] > 0 for e in events)
    return sum(e["submit_us"] for e in events)


if __name__ == "__main__":
    results = {}
    for size in (32, 64):
        indexed, reference = [], []
        for iteration in range(20):
            modes = ("indexpair", "indexpairref") if iteration % 2 == 0 else (
                "indexpairref", "indexpair")
            for mode in modes:
                (indexed if mode == "indexpair" else reference).append(
                    measure(size, mode))
        results[size] = {
            "indexed_two_submissions_us": indexed,
            "nonindexed_one_submission_us": reference,
            "indexed_median_us": statistics.median(indexed),
            "nonindexed_median_us": statistics.median(reference),
        }
        print(f"N100 {size}: indexed {results[size]['indexed_median_us']} us, "
              f"reference {results[size]['nonindexed_median_us']} us median",
              flush=True)
    (HERE / "pair-submit-times.json").write_text(json.dumps({
        "scope": "sum of vk_queue_submit submit_us per process; blocking owner ioctl and wait, not frame latency or isolated GPU time",
        "runs_per_mode_per_size": 20,
        "results": results,
    }, indent=2) + "\n")
