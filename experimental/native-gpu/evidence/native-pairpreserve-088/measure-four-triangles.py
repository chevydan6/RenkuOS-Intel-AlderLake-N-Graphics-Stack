#!/usr/bin/env python3
"""Interleave baseline and preserving-pair Vulkan draws on the physical N100."""
import json
from pathlib import Path
import statistics
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-pairpreserve-088"
DEPS = ":/boot/home/native-blake3-080:/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib"
EXPECTED = {32: "5bf758b9", 64: "61162dae"}


def measure(size, kind, trial):
    prior = kind == "baseline"
    libraries = ("/boot/home/native-draw9-087" if prior else BUNDLE) + DEPS
    program = BUNDLE + ("/baseline_probe" if prior else "/triangle_probe")
    mode = "draw3x4" if prior else "draw12"
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    command = (f"LIBRARY_PATH={libraries} {target}RENKU_VK_TRACE=1 "
               f"timeout 20 {program} 0 original 4 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    versions = [20, 19, 19] if prior else [20, 21]
    assert result.returncode == 0 and len(events) == len(versions), result
    assert [event["version"] for event in events] == versions, events
    assert f"hash={EXPECTED[size]}" in result.stdout, result.stdout
    assert all(event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
               and event["sequence"] == event["observed"] > 0
               and event["matched"] == event["checked"] > 0
               for event in events), events
    assert events[-1]["pixel_hash"] == EXPECTED[size]
    return {"size": size, "kind": kind, "trial": trial,
            "submit_us": sum(event["submit_us"] for event in events),
            "events": events, "stdout": result.stdout.strip()}


if __name__ == "__main__":
    records = []
    for size in (32, 64):
        for trial in range(20):
            order = ("baseline", "pair") if trial % 2 == 0 else ("pair", "baseline")
            for kind in order:
                records.append(measure(size, kind, trial))
        print(f"N100 four-triangle benchmark: PASS size={size} trials=20", flush=True)
    summary = {}
    for size in (32, 64):
        baseline = statistics.median(r["submit_us"] for r in records
                                     if r["size"] == size and r["kind"] == "baseline")
        pair = statistics.median(r["submit_us"] for r in records
                                 if r["size"] == size and r["kind"] == "pair")
        summary[str(size)] = {"baseline_median_submit_us": baseline,
                              "pair_median_submit_us": pair,
                              "reduction_percent": round(100 * (baseline-pair)/baseline, 1)}
    (HERE / "samples.json").write_text(json.dumps(records, indent=2) + "\n")
    (HERE / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, sort_keys=True))
