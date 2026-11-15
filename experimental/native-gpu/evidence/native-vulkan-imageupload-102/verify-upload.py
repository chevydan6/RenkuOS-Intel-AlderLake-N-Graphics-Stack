#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compare N100 upload captures with independent triangle-only captures."""
from hashlib import sha256
import json
from pathlib import Path

here = Path(__file__).resolve().parent
reference = here.parent / "native-multidraw-082"
poison = (0x19537BAD).to_bytes(4, "little")
background = (0xFF336699).to_bytes(4, "little")
for width in (32, 64):
    old = (reference / f"single{width}-second.bin").read_bytes()
    actual = (here / f"capture-{width}.bin").read_bytes()
    assert len(old) == len(actual) == width * width * 4
    expected = bytearray(old)
    for offset in range(0, len(expected), 4):
        if expected[offset : offset + 4] == poison:
            expected[offset : offset + 4] = background
    mismatches = sum(
        expected[offset : offset + 4] != actual[offset : offset + 4]
        for offset in range(0, len(expected), 4)
    )
    print(json.dumps({"width": width, "pixels": width * width,
                      "mismatches": mismatches,
                      "sha256": sha256(actual).hexdigest()},
                     separators=(",", ":")))
    assert mismatches == 0
