#!/usr/bin/env python3
"""Check the complete N100 indexed mesh image against its UBO variant."""
from collections import Counter
from pathlib import Path
from struct import unpack

here = Path(__file__).resolve().parent
reference = (here / "indexmesh-reference.bgra").read_bytes()
uniform = (here / "indexmesh-ubo.bgra").read_bytes()
assert len(reference) == len(uniform) == 32 * 32 * 4
reference_pixels = unpack("<1024I", reference)
uniform_pixels = unpack("<1024I", uniform)
def fnv32(pixels):
    value = 2166136261
    for pixel in pixels:
        value = ((value ^ pixel) * 16777619) & 0xFFFFFFFF
    return value

assert fnv32(reference_pixels) == 0xCDAD4A41
assert fnv32(uniform_pixels) == 0xF2927141
assert Counter(reference_pixels) == {0x19537BAD: 588, 0xFFFF4000: 436}
assert Counter(uniform_pixels) == {0x19537BAD: 588, 0xFF00FF80: 436}
assert all(
    actual == (0xFF00FF80 if expected == 0xFFFF4000 else expected)
    for expected, actual in zip(reference_pixels, uniform_pixels)
)
print("N100 indexed UBO image: PASS (436 shader pixels changed, 588 background pixels preserved)")
