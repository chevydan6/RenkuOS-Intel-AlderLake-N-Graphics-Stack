#!/usr/bin/env python3
"""Check saved N100 traces for the native Mesa GL-header closure experiment."""
import json
from pathlib import Path
import re

root = Path(__file__).parent


def check(name, expected_cases, expected_draws, direct):
    cases = []
    sequences = set()
    current = None
    for line in (root / name).read_text().splitlines():
        if line.startswith("CASE "):
            assert "rc=0" in line, line
            repeats = re.search(r"repeats=(\d+)", line)
            current = {"repeats": int(repeats[1]) if repeats else 4,
                       "draws": 0, "label": line}
            cases.append(current)
        elif ": PASS" in line:
            assert current is not None
            match = re.search(r"hash=([0-9a-f]{8}) draws=(\d+)", line)
            assert match and int(match[2]) == current["repeats"], line
            current["hash"] = match[1]
        elif line.startswith("{"):
            assert current is not None and "hash" in current
            event = json.loads(line)
            assert event["event"] == "vk_queue_submit"
            assert event["direct_target"] == int(direct)
            assert event["version"] == (15 if direct else (7 if "32" in current["label"] else 10))
            assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
            assert event["sequence"] == event["observed"] > 0
            assert event["sequence"] not in sequences
            assert event["checked"] == event["matched"] > 0
            if "variant=dual " in current["label"] or "variant=multipass " in current["label"]:
                expected = current["hash"] if current["draws"] else "28616d2d"
            else:
                expected = current["hash"]
            assert event["pixel_hash"] == expected
            if direct:
                assert event["write_ioctl_us"] == 0
            sequences.add(event["sequence"])
            current["draws"] += 1
        else:
            assert not line.startswith("Vulkan triangle failed"), line
    assert len(cases) == expected_cases and len(sequences) == expected_draws
    assert all(c["draws"] == c["repeats"] for c in cases)


check("physical-matrix.log", 39, 124, True)
check("supplemental-matrix.log", 9, 30, True)
check("fallback.log", 2, 8, False)
print("Native GL-header N100 evidence: PASS (50 cases, 162 draws)")
