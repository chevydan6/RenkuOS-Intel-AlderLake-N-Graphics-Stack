#!/usr/bin/env python3
"""Audit the saved version-14 N100 regression matrix."""
import json
import pathlib
import re

lines = pathlib.Path(__file__).with_name("physical-matrix.log").read_text().splitlines()
cases = []
events = []
case = None
for line in lines:
    if line.startswith("CASE "):
        m = re.fullmatch(
            r"CASE (32|64) profile=(\w+) variant=(\w+) repeats=(\d+) rc=(\d+)",
            line)
        assert m and m[5] == "0", line
        case = dict(size=int(m[1]), profile=m[2], variant=m[3],
                    repeats=int(m[4]), draws=0)
        cases.append(case)
    elif "triangle: PASS" in line:
        assert case is not None
        m = re.search(r"hash=([0-9a-f]{8}) draws=(\d+)", line)
        assert m and int(m[2]) == case["repeats"], line
        case["hash"] = m[1]
    elif line.startswith("{"):
        assert case is not None
        event = json.loads(line)
        assert event["event"] == "vk_queue_submit"
        variant = case["variant"]
        version = (14 if variant in ("rotateoffset", "rotatefirst",
                                     "rotatepage", "depthoffset") else
                   13 if variant in ("offset", "pageoffset") else
                   11 if variant in ("rotate", "depth") else
                   12 if case["profile"] == "fragcoord128" else
                   7 if case["size"] == 32 else 10)
        offset = 4096 if variant in ("rotatepage", "pageoffset") else (
            48 if variant in ("rotateoffset", "rotatefirst", "depthoffset",
                              "offset") else 0)
        assert event["version"] == version and event["vertex_offset"] == offset
        assert bool(event["vertex_shader_bo"]) == (version in (11, 14))
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == case["hash"]
        case["draws"] += 1
        events.append(event)
    else:
        assert not line.startswith("Vulkan triangle failed"), line

assert len(cases) == 15 and len(events) == 60
assert all(c["draws"] == c["repeats"] for c in cases)
assert len({e["sequence"] for e in events}) == len(events)
assert sum(e["version"] == 14 for e in events) == 28
print("N100 distinct vertex shader offsets: PASS (15 cases, 60 draws)")
