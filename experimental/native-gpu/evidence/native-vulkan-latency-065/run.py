#!/usr/bin/env python3
"""Measure repeated Vulkan triangle submissions on the physical N100."""
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-semaphore-064"
LIBS = (BUNDLE + ":/boot/home/native-rcs-large-039/vulkan-deps:"
        "/boot/home/config/lib:/boot/system/lib")
REPEATS = 64
RUNS = 3


def percentile(values, fraction):
    ordered = sorted(values)
    return ordered[int(fraction * (len(ordered) - 1))]


summary = {"device": "Intel N100", "bundle": BUNDLE, "draws_per_run": REPEATS,
           "runs_per_case": RUNS, "cases": []}
for size in (32, 64):
    for direct in (True, False):
        events = []
        seen = set()
        for run in range(RUNS):
            command = (f"LIBRARY_PATH={LIBS} RENKU_VK_TRACE=1 "
                       f"RENKU_VK_DIRECT_TARGET={int(direct)} "
                       + ("RENKU_VK_TARGET_64=1 " if size == 64 else "")
                       + f"{BUNDLE}/triangle_probe 0 original {REPEATS}")
            result = subprocess.run(["ssh", "renkuos-n100", command],
                                    capture_output=True, text=True, timeout=60)
            label = f"{size}-{int(direct)}-{run}"
            (HERE / f"{label}.log").write_text(
                f"rc={result.returncode}\n{result.stdout}{result.stderr}")
            current = [json.loads(line) for line in result.stderr.splitlines()
                       if line.startswith('{') and '"event":"vk_queue_submit"' in line]
            expected_hash = "28616d2d" if size == 32 else "8aaee2f5"
            assert result.returncode == 0 and len(current) == REPEATS, label
            assert f"hash={expected_hash} draws={REPEATS}" in result.stdout, label
            for item in current:
                assert item["sequence"] == item["observed"] > 0, label
                assert item["sequence"] not in seen, label
                assert item["pixel_hash"] == expected_hash, label
                assert item["checked"] == item["matched"] > 0, label
                assert item["ioctl"] == item["write_ioctl"] == item["fault"] == 0, label
                assert item["direct_target"] == int(direct), label
                seen.add(item["sequence"])
            events.extend(current)
        measures = {}
        for key in ("render_ioctl_us", "write_ioctl_us", "submit_us"):
            samples = [item[key] for item in events]
            measures[key] = {"median": statistics.median(samples),
                             "p95": percentile(samples, .95),
                             "min": min(samples), "max": max(samples)}
        row = {"size": size, "direct": direct, "draws": len(events),
               "measurements_us": measures}
        summary["cases"].append(row)
        print(row, flush=True)
(HERE / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print("N100 Vulkan latency: PASS (768 verified draws)")
