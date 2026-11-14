#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check Mesa 26.3 N100 packet deltas before any larger-target hardware work."""
import argparse
from pathlib import Path
import re


def words(source, name):
    body = re.search(r"static constexpr uint32_t " + name
                     + r"(?:\[\d*\])? = \{([^}]*)\}", source, re.S)
    if body is None:
        raise ValueError(f"missing {name} array")
    return [int(word, 16) for word in re.findall(r"0x([0-9a-fA-F]+)U", body[1])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet32", type=Path)
    parser.add_argument("packet64", type=Path)
    args = parser.parse_args()
    source32, source64 = args.packet32.read_text(), args.packet64.read_text()
    expected = {
        "Batch": {2: (0x00407000, 0x0040a000),
                  25: (0x00407000, 0x0040a000),
                  41: (0x00008001, 0x0000b001),
                  44: (0x00008001, 0x0000b001),
                  53: (0x00407008, 0x0040a008),
                  290: (0x001f001f, 0x003f003f)},
        "Dynamic": {0: (0x41800000, 0x42000000),
                    1: (0x41800000, 0x42000000),
                    3: (0x41800000, 0x42000000),
                    4: (0x41800000, 0x42000000),
                    13: (0x41f80000, 0x427c0000),
                    15: (0x41f80000, 0x427c0000),
                    65: (0x001f001f, 0x003f003f)},
        "Surface": {18: (0x001f001f, 0x003f003f),
                    19: (0x0000007f, 0x000000ff)},
        "Vs": {}, "Fs": {},
    }
    for name, changes in expected.items():
        before, after = words(source32, name), words(source64, name)
        actual = {index: (left, right)
                  for index, (left, right) in enumerate(zip(before, after))
                  if left != right}
        if len(before) != len(after) or actual != changes:
            raise ValueError(f"unexpected {name} packet delta: {actual}")
    print("Mesa 26.3 N100 32/64 packet delta: PASS")


if __name__ == "__main__":
    main()
