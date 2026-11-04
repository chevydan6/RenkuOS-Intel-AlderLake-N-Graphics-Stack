#!/usr/bin/env python3
"""Validate actual GART table coverage; no hardware accesses or driver execution."""
import pathlib
import re
import sys

root = pathlib.Path(sys.argv[1])
source = (root / 'src/add-ons/kernel/busses/agp_gart/intel_gart.cpp').read_text()
table = source.split('} kSupportedDevices[] = {', 1)[1].split('\n};', 1)[0]
rows = re.findall(r'\{(0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+),\s*(\w+),\s*"([^"]+)"\}', table)
pairs = {(int(b, 16), int(d, 16)): model for b, d, model, _ in rows}
checks = [
    ('verified N100 pair', pairs.get((0x461c, 0x46d1)) == 'INTEL_MODEL_ALDM'),
    ('existing Alder Lake pair preserved', pairs.get((0x4621, 0x46d1)) == 'INTEL_MODEL_ALDM'),
    ('existing Alder Lake-P preserved', pairs.get((0x4621, 0x46a6)) == 'INTEL_MODEL_ALDM'),
    ('no speculative 46d0', (0x461c, 0x46d0) not in pairs),
    ('no speculative 46d2', (0x461c, 0x46d2) not in pairs),
    ('QEMU bridge not treated as N100', (0x1237, 0x46d1) not in pairs),
    ('QEMU VGA not treated as N100', (0x461c, 0x1111) not in pairs),
    ('N100 pair unique', sum(int(b, 16) == 0x461c and int(d, 16) == 0x46d1 for b,d,_,_ in rows) == 1),
]
for name, passed in checks:
    print(('PASS ' if passed else 'FAIL ') + name)
raise SystemExit(0 if all(passed for _, passed in checks) else 1)
