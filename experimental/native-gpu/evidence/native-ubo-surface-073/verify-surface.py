#!/usr/bin/env python3
"""Decode the Gen12 UBO state against Mesa XML, independent of pack macros."""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: verify-surface.py MESA_26_3_SOURCE")
mesa = Path(sys.argv[1]).resolve()
sys.path.insert(0, str(mesa / "src/intel/genxml"))
from intel_genxml import GenXml  # noqa: E402

xml = GenXml(mesa / "src/intel/genxml/gen120.xml", import_xml=True).et.getroot()
nodes = {node.attrib["name"]: node for node in xml if "name" in node.attrib}
lines = Path(__file__).with_name("surface-words.log").read_text().splitlines()
words = []
for index, line in enumerate(lines):
    number, value = line.split()
    assert int(number) == index
    words.append(int(value, 16))
assert len(words) == 16


def field(name):
    item = next(f for f in nodes["RENDER_SURFACE_STATE"]
                if f.tag == "field" and f.attrib["name"] == name)
    high, low = map(int, item.attrib["bits"].split(":"))
    start = int(item.attrib["dword"]) * 32 + low
    value = (sum(word << (32 * i) for i, word in enumerate(words)) >> start)
    value &= (1 << (high - low + 1)) - 1
    return value << low if item.attrib["type"] == "address" else value


expected = {"Surface Type": 4, "Surface Format": 511, "Width": 15,
            "Height": 0, "Surface Pitch": 0, "MOCS": 6,
            "Surface Base Address": 0x410000, "Auxiliary Surface Mode": 0,
            "Memory Compression Enable": 0, "Tile Mode": 0}
for name, value in expected.items():
    assert field(name) == value, (name, field(name), value)
print("Mesa XML UBO surface: PASS (RAW 16 bytes at 0x410000)")
