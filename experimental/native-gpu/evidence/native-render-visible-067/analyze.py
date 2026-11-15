#!/usr/bin/env python3
"""Match owner stage timings with Vulkan traces by hardware sequence ID."""
import json
from pathlib import Path
import statistics

HERE = Path(__file__).resolve().parent
STAGES = ("accepted_us", "forcewake_us", "group_before_work_us",
          "work_to_publish_us", "csb_wait_us", "verify_us",
          "group_after_work_us", "finish_us")
owner = {}
for line in (HERE / "owner-timings.log").read_text().splitlines():
    assert line.startswith("KERN: native_rcs_timing ")
    fields = dict(part.split("=", 1) for part in line.split()[2:])
    item = {key: int(value) for key, value in fields.items()}
    sequence = item.pop("seq")
    assert sequence not in owner and sequence > 0
    assert set(item) == set(STAGES) | {"total_us", "visible_us", "visible_calls"}
    assert sum(item[key] for key in STAGES) == item["total_us"]
    assert item["visible_calls"] == 3 and 0 < item["visible_us"] < item["total_us"]
    owner[sequence] = item
assert len(owner) == 32


def stats(values):
    ordered = sorted(values)
    return {"median": statistics.median(ordered),
            "p95": ordered[int(.95 * (len(ordered) - 1))],
            "min": ordered[0], "max": ordered[-1]}


result = {"draws": 32, "cases": []}
used = set()
for size, expected in ((32, "28616d2d"), (64, "8aaee2f5")):
    data = (HERE / f"triangle{size}.log").read_text()
    assert f"hash={expected} draws=16" in data
    events = [json.loads(line) for line in data.splitlines()
              if line.startswith('{') and '"event":"vk_queue_submit"' in line]
    assert len(events) == 16
    samples = []
    for event in events:
        sequence = event["sequence"]
        assert sequence == event["observed"] and sequence in owner
        assert sequence not in used
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == expected and event["direct_target"] == 1
        assert event["render_ioctl_us"] >= owner[sequence]["total_us"]
        used.add(sequence)
        samples.append({**owner[sequence],
                        "ioctl_remainder_us": event["render_ioctl_us"] - owner[sequence]["total_us"],
                        "render_ioctl_us": event["render_ioctl_us"],
                        "submit_us": event["submit_us"]})
    result["cases"].append({"size": size, "draws": len(samples),
                            "measurements_us": {key: stats([sample[key] for sample in samples])
                                                for key in (*STAGES, "total_us", "visible_us", "visible_calls",
                                                            "ioctl_remainder_us",
                                                            "render_ioctl_us", "submit_us")}})
assert used == set(owner)
print(json.dumps(result, indent=2))
