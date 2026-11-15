#!/usr/bin/env python3
"""Interleave physical N100 compilation samples for xxHash source pruning."""
import json
from pathlib import Path
import re
import statistics
import subprocess


HERE = Path(__file__).resolve().parent
LIBRARIES = "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib"
PATHS = {"baseline": "/boot/home/native-fourcc-090",
         "candidate": "/boot/home/native-xxhash-092"}


def sample(label):
    root = PATHS[label]
    command = f"LIBRARY_PATH={LIBRARIES} {root}/compiler-bench 100"
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 0 and not result.stderr, result
    assert f"compiler={root}/librenku_intel_compiler.so" in result.stdout, result
    match = re.search(r"iterations=100 elapsed_us=(\d+) checksum=(\d+)",
                      result.stdout)
    assert match and int(match[2]) == 48200, result
    return int(match[1])


if __name__ == "__main__":
    sample("baseline");sample("candidate")
    rows = []
    for index in range(20):
        order = ("baseline", "candidate") if index % 2 == 0 else (
            "candidate", "baseline")
        for label in order:
            elapsed = sample(label)
            rows.append({"round": index, "compiler": label,
                         "iterations": 100, "elapsed_us": elapsed})
    medians = {label: statistics.median(row["elapsed_us"] for row in rows
                                   if row["compiler"] == label)
               for label in PATHS}
    report = {"samples": rows, "median_us": medians,
              "candidate_over_baseline": medians["candidate"] / medians["baseline"]}
    (HERE / "compiler-benchmark.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"median_us": medians,
                      "candidate_over_baseline": report["candidate_over_baseline"]}))
