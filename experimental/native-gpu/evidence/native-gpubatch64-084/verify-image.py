#!/usr/bin/env python3
"""Compare complete N100 batch images with independent single draws."""
from pathlib import Path
import struct

here = Path(__file__).resolve().parent
reference = here.parent / "native-multidraw-082"
poison = 0x19537bad
red = 0xffff4000
for size in (32, 64):
    count = size * size
    def words(path):
        data = path.read_bytes()
        assert len(data) == 4 * count, (path, len(data))
        return struct.unpack(f"<{count}I", data)
    first = words(reference / f"single{size}-first.bin")
    second = words(reference / f"single{size}-second.bin")
    actual = words(here / f"batch{size}.bin")
    expected = [red if a != poison or b != poison else poison
                for a, b in zip(first, second)]
    mismatches = [i for i, (want, got) in enumerate(zip(expected, actual))
                  if want != got]
    print(f"{size}x{size}: {len(mismatches)} mismatches / {count} pixels")
    assert not mismatches, mismatches[:8]
