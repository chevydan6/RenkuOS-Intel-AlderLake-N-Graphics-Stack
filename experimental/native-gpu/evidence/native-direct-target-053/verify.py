#!/usr/bin/env python3
"""Audit saved N100 direct-target traces and the A/B timing sample."""
import json
from pathlib import Path
import re

root = Path(__file__).parent

def check_matrix(name, expected_cases, expected_draws, direct_cases):
    cases = []
    events = []
    case = None
    for line in (root / name).read_text().splitlines():
        if line.startswith("CASE "):
            assert "rc=0" in line, line
            repeat = re.search(r"repeats=(\d+)", line)
            case = dict(label=line, repeats=int(repeat[1]) if repeat else 4,
                        draws=0, direct=len(cases) < direct_cases)
            cases.append(case)
        elif ": PASS" in line:
            assert case is not None
            match = re.search(r"hash=([0-9a-f]{8}) draws=(\d+)", line)
            assert match and int(match[2]) == case["repeats"], line
            case["hash"] = match[1]
        elif line.startswith("{"):
            assert case is not None
            event = json.loads(line)
            assert event["event"] == "vk_queue_submit"
            assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
            assert event["sequence"] == event["observed"] > 0
            assert event["checked"] == event["matched"] > 0
            if "variant=dual " in case["label"] or "variant=multipass " in case["label"]:
                expected_hash = case["hash"] if case["draws"] else "28616d2d"
            else:
                expected_hash = case["hash"]
            assert event["pixel_hash"] == expected_hash
            assert event["direct_target"] == int(case["direct"])
            if case["direct"]:
                assert event["version"] == 15 and event["write_ioctl_us"] == 0
            case["draws"] += 1
            events.append(event)
        else:
            assert not line.startswith("Vulkan triangle failed"), line
    assert len(cases) == expected_cases and len(events) == expected_draws
    assert all(c["draws"] == c["repeats"] for c in cases)
    assert len({e["sequence"] for e in events}) == len(events)
    return len(cases), len(events)

assert check_matrix("physical-matrix.log", 39, 124, 39) == (39, 124)
assert check_matrix("supplemental-matrix.log", 9, 30, 9) == (9, 30)
assert check_matrix("default-check.log", 6, 24, 4) == (6, 24)
profile = json.loads((root / "ab-profile.json").read_text())
assert [r["mode"] for r in profile["runs"]] == ["copy", "direct", "copy", "direct"]
for run in profile["runs"]:
    assert len(run["events"]) == 32 and "PASS" in run["pass_line"]
    assert all(e["direct_target"] == int(run["mode"] == "direct")
               and e["checked"] == e["matched"] > 0
               and e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
               for e in run["events"])
print("N100 direct target evidence: PASS (39 + 9 + 6 cases, 178 regression draws; 128 A/B draws)")
