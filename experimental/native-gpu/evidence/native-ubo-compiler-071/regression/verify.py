#!/usr/bin/env python3
"""Check saved N100 semaphore and prior rendering traces."""
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
    return sequences


all_sequences = set()
for spec in (("physical-matrix.log", 39, 124, True),
             ("supplemental-matrix.log", 9, 30, True),
             ("fallback.log", 2, 8, False)):
    sequences = check(*spec)
    assert not (all_sequences & sequences)
    all_sequences.update(sequences)

expected_hashes = {"fragcoordgreen32": "6e59daf5",
                   "fragcoordgreen": "f25bcaf5",
                   "fragcoordgreen128": "649956f5"}
cases = []
current = None
for line in (root / "green-matrix.log").read_text().splitlines():
    if line.startswith("CASE "):
        assert "rc=0" in line
        profile = re.search(r"profile=(\w+)", line)
        direct = re.search(r"direct=([01])", line)
        assert profile and direct and profile[1] in expected_hashes
        current = {"hash": expected_hashes[profile[1]],
                   "direct": int(direct[1]), "draws": 0}
        cases.append(current)
    elif ": PASS" in line:
        assert current is not None
        assert f"hash={current['hash']} draws=4" in line
    elif line.startswith("{"):
        assert current is not None
        event = json.loads(line)
        assert event["event"] == "vk_queue_submit"
        assert event["direct_target"] == current["direct"]
        assert event["version"] == (15 if current["direct"] else 12)
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["checked"] == event["matched"] == 3476
        assert event["pixel_hash"] == current["hash"]
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in all_sequences
        if current["direct"]:
            assert event["write_ioctl_us"] == 0
        all_sequences.add(event["sequence"])
        current["draws"] += 1
assert len(cases) == 4 and all(c["draws"] == 4 for c in cases)
assert len(all_sequences) == 178

cases = []
current = None
for line in (root / "semaphore-matrix.log").read_text().splitlines():
    if line.startswith("CASE "):
        assert "rc=0" in line
        size = re.search(r"CASE (32|64) ", line)
        direct = re.search(r"direct=([01])", line)
        assert size and direct
        current = {"size": int(size[1]), "direct": int(direct[1]),
                   "draws": 0, "hash": None}
        cases.append(current)
    elif ": PASS" in line:
        assert current is not None
        match = re.search(r"hash=([0-9a-f]{8}) draws=2", line)
        assert match
        current["hash"] = match[1]
    elif line.startswith("{"):
        assert current is not None and current["hash"]
        event = json.loads(line)
        assert event["event"] == "vk_queue_submit"
        assert event["direct_target"] == current["direct"]
        assert event["version"] == (15 if current["direct"] else
                                    7 if current["size"] == 32 else 10)
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["checked"] == event["matched"] > 0
        assert event["pixel_hash"] == current["hash"]
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in all_sequences
        if current["direct"]:
            assert event["write_ioctl_us"] == 0
        all_sequences.add(event["sequence"])
        current["draws"] += 1
assert len(cases) == 14 and all(c["draws"] == 2 for c in cases)
assert len(all_sequences) == 206
print("N100 binary semaphore: PASS (68 cases, 206 draws)")
