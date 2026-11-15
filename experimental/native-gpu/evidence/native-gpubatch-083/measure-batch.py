#!/usr/bin/env python3
"""Measure one GPU batch versus two blocking RCS publications on N100."""
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-gpubatch-083"
LIBS = (BUNDLE + ":/boot/home/native-blake3-080:"
        "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
records = []
for sample in range(30):
    for name, setting in (("batch", ""), ("fallback", "RENKU_VK_NATIVE_BATCH=0 ")):
        command = (f"LIBRARY_PATH={LIBS} RENKU_VK_DIRECT_TARGET=1 "
                   f"{setting}RENKU_VK_TRACE=1 timeout 20 "
                   f"{BUNDLE}/triangle_probe 0 original 2 batchgpu")
        result = subprocess.run(["ssh", "renkuos-n100", command],
                                capture_output=True, text=True, timeout=30)
        events = [json.loads(line) for line in result.stderr.splitlines()
                  if line.startswith('{') and '"event":"vk_queue_submit"' in line]
        expected = [20] if name == "batch" else [15, 19]
        if (result.returncode or "PASS hash=d23b1ba1" not in result.stdout
                or [event["version"] for event in events] != expected
                or any(event["ioctl"] or event["fault"]
                       or event["matched"] != event["checked"] for event in events)
                or events[-1]["pixel_hash"] != "d23b1ba1"):
            raise RuntimeError((sample, name, result.returncode,
                                result.stdout, result.stderr))
        records.append({"sample": sample, "variant": name,
                        "submissions": len(events),
                        "total_submit_us": sum(event["submit_us"] for event in events),
                        "individual_submit_us": [event["submit_us"] for event in events]})

summary = {}
for name in ("batch", "fallback"):
    values = [row["total_submit_us"] for row in records
              if row["variant"] == name]
    summary[name] = {"samples": len(values), "median_submit_us": statistics.median(values),
                     "minimum_submit_us": min(values),
                     "maximum_submit_us": max(values)}
summary["median_reduction_percent"] = round(100 * (1 -
    summary["batch"]["median_submit_us"] / summary["fallback"]["median_submit_us"]), 1)
(HERE / "batch-samples.json").write_text(json.dumps(records, indent=2) + "\n")
(HERE / "batch-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
