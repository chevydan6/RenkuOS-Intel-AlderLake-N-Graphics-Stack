#!/usr/bin/env python3
"""Check the saved physical N100 vertex-offset regression trace."""
import json
import pathlib
import re

path = pathlib.Path(__file__).with_name("physical-matrix.log")
lines = path.read_text().splitlines()
cases = []
events = []
current = None
for line in lines:
    if line.startswith("CASE "):
        match = re.fullmatch(
            r"CASE (32|64) shape=(\d) profile=(\w+) repeats=(\d+) "
            r"variant=(\w+) rc=(\d+)", line)
        assert match and match[6] == "0", line
        current = dict(size=int(match[1]), shape=int(match[2]),
                       profile=match[3], repeats=int(match[4]),
                       variant=match[5], draws=0)
        cases.append(current)
    elif '"event":"vk_queue_submit"' in line:
        assert current is not None
        event = json.loads(line)
        expected_offset = {"offset": 48, "firstvertex": 48,
                           "pageoffset": 4096}.get(current["variant"], 0)
        expected_version = (13 if expected_offset else
                            7 if current["size"] == 32 else
                            12 if current["profile"].startswith("fragcoord") else
                            11 if current["variant"] in ("rotate", "depth") else 10)
        assert event["version"] == expected_version, (current, event)
        assert event["vertex_offset"] == expected_offset, (current, event)
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"]
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == current["hash"]
        current["draws"] += 1
        events.append(event)
    elif "triangle: PASS" in line:
        assert current is not None
        hash_match = re.search(r"hash=([0-9a-f]{8})", line)
        assert hash_match, line
        current["hash"] = hash_match[1]
        assert f'draws={current["repeats"]}' in line
    else:
        assert not line.startswith("Vulkan triangle failed"), line

assert len(cases) == 34 and len(events) == 112, (len(cases), len(events))
assert all(case["draws"] == case["repeats"] for case in cases), cases
assert {event["vertex_offset"] for event in events} == {0, 48, 4096}
print(f"N100 vertex-offset matrix: PASS ({len(cases)} cases, {len(events)} draws)")
