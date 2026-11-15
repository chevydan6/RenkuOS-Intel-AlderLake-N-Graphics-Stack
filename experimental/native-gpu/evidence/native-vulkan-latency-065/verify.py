#!/usr/bin/env python3
"""Recompute the latency summary from preserved N100 Vulkan traces."""
import json
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
summary = json.loads((HERE / "summary.json").read_text())
assert summary["draws_per_run"] == 64 and summary["runs_per_case"] == 3
assert len(summary["cases"]) == 4
all_sequences = set()
for row in summary["cases"]:
    size, direct = row["size"], row["direct"]
    assert size in (32, 64) and type(direct) is bool
    events = []
    expected_hash = "28616d2d" if size == 32 else "8aaee2f5"
    for run in range(3):
        content = (HERE / f"{size}-{int(direct)}-{run}.log").read_text()
        assert content.startswith("rc=0\n")
        assert f"hash={expected_hash} draws=64" in content
        current = [json.loads(line) for line in content.splitlines()
                   if line.startswith('{') and '"event":"vk_queue_submit"' in line]
        assert len(current) == 64
        for item in current:
            sequence = item["sequence"]
            assert sequence == item["observed"] > 0
            assert sequence not in all_sequences
            all_sequences.add(sequence)
            assert item["pixel_hash"] == expected_hash
            assert item["checked"] == item["matched"] > 0
            assert item["ioctl"] == item["write_ioctl"] == item["fault"] == 0
            assert item["direct_target"] == int(direct)
        events.extend(current)
    assert len(events) == row["draws"] == 192
    for key, recorded in row["measurements_us"].items():
        values = sorted(item[key] for item in events)
        assert recorded == {"median": statistics.median(values),
                            "p95": values[int(.95 * (len(values) - 1))],
                            "min": values[0], "max": values[-1]}
assert len(all_sequences) == 768
print("N100 Vulkan latency traces: PASS (768 distinct verified draws)")
