#!/usr/bin/env python3
"""Interleave one-publication and two-publication triangle pairs on N100."""
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-gpubatch64-084"
LIBS = (BUNDLE + ":/boot/home/native-blake3-080:"
        "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
records = []
for size, expected_hash in ((32, "d23b1ba1"), (64, "bb1f873b")):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    for sample in range(20):
        for name, setting in (("batch", ""), ("fallback", "RENKU_VK_NATIVE_BATCH=0 ")):
            command = (f"LIBRARY_PATH={LIBS} RENKU_VK_DIRECT_TARGET=1 "
                       f"{target}{setting}RENKU_VK_TRACE=1 timeout 20 "
                       f"{BUNDLE}/triangle_probe 0 original 2 batchgpu")
            result = subprocess.run(["ssh", "renkuos-n100", command],
                                    capture_output=True, text=True, timeout=30)
            events = [json.loads(line) for line in result.stderr.splitlines()
                      if line.startswith('{') and '"event":"vk_queue_submit"' in line]
            versions = [20] if name == "batch" else [15, 19]
            if (result.returncode or f"PASS hash={expected_hash}" not in result.stdout
                    or [event["version"] for event in events] != versions
                    or any(event["ioctl"] or event["fault"]
                           or event["matched"] != event["checked"] for event in events)
                    or events[-1]["pixel_hash"] != expected_hash):
                raise RuntimeError((size, sample, name, result.returncode,
                                    result.stdout, result.stderr))
            records.append({"size": size, "sample": sample, "variant": name,
                            "submissions": len(events),
                            "total_submit_us": sum(event["submit_us"] for event in events)})

summary = {}
for size in (32, 64):
    summary[str(size)] = {}
    for name in ("batch", "fallback"):
        values = [row["total_submit_us"] for row in records
                  if row["size"] == size and row["variant"] == name]
        summary[str(size)][name] = {"samples": len(values),
            "median_submit_us": statistics.median(values),
            "minimum_submit_us": min(values), "maximum_submit_us": max(values)}
    fast = summary[str(size)]["batch"]["median_submit_us"]
    slow = summary[str(size)]["fallback"]["median_submit_us"]
    summary[str(size)]["median_reduction_percent"] = round(100 * (1 - fast / slow), 1)
(HERE / "batch-samples.json").write_text(json.dumps(records, indent=2) + "\n")
(HERE / "batch-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
