#!/usr/bin/env python3
"""Interleave repeated and two-draw Vulkan submits on the physical N100."""
import json
from pathlib import Path
import statistics
import subprocess

HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-multidraw-082"
LIBS = (BUNDLE + ":/boot/home/native-blake3-080:"
        "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
SAMPLES = 20
records = []

for size in (32, 64):
    for sample in range(SAMPLES):
        for variant in (("repeat", ""), ("multidraw", " multidraw")):
            name, suffix = variant
            setting = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
            command = (f"LIBRARY_PATH={LIBS} RENKU_VK_DIRECT_TARGET=1 "
                       f"RENKU_VK_TRACE=1 {setting}timeout 20 "
                       f"{BUNDLE}/triangle_probe 0 original 2{suffix}")
            result = subprocess.run(["ssh", "renkuos-n100", command],
                                    capture_output=True, text=True, timeout=30)
            events = [json.loads(line) for line in result.stderr.splitlines()
                      if line.startswith('{') and '"event":"vk_queue_submit"' in line]
            if result.returncode or len(events) != 2 or "PASS" not in result.stdout:
                raise RuntimeError((size, sample, name, result.returncode,
                                    result.stdout, result.stderr))
            if any(event["ioctl"] or event["fault"] or
                   event["checked"] != event["matched"] for event in events):
                raise RuntimeError((size, sample, name, events))
            expected = [15, 15] if name == "repeat" else [15, 19]
            if [event["version"] for event in events] != expected:
                raise RuntimeError((size, sample, name, events))
            records.append({"size": size, "sample": sample, "variant": name,
                            "submit_us": [event["submit_us"] for event in events],
                            "total_submit_us": sum(event["submit_us"]
                                                   for event in events),
                            "final_hash": events[-1]["pixel_hash"]})

summary = {}
for size in (32, 64):
    summary[str(size)] = {}
    for name in ("repeat", "multidraw"):
        subset = [record for record in records
                  if record["size"] == size and record["variant"] == name]
        totals = [record["total_submit_us"] for record in subset]
        first = [record["submit_us"][0] for record in subset]
        second = [record["submit_us"][1] for record in subset]
        summary[str(size)][name] = {
            "samples": len(subset), "median_total_submit_us": statistics.median(totals),
            "median_first_submit_us": statistics.median(first),
            "median_second_submit_us": statistics.median(second),
            "min_total_submit_us": min(totals), "max_total_submit_us": max(totals),
        }

(HERE / "submit-samples.json").write_text(json.dumps(records, indent=2) + "\n")
(HERE / "submit-summary.json").write_text(json.dumps(summary, indent=2) + "\n")
print(json.dumps(summary, indent=2))
