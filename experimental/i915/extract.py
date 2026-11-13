#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Retain original v6.6 function bodies; do not replace hardware algorithms."""
from pathlib import Path
import hashlib
import re

root = Path(__file__).resolve().parent
source = root / 'upstream/gt/intel_ring.c'
data = source.read_text()
assert hashlib.sha1(b'blob ' + str(len(source.read_bytes())).encode()
                    + b'\0' + source.read_bytes()).hexdigest() == '59da4b7bd262058855d18023bbec71e8e0618793'
parts = []
for name in ['intel_ring_update_space', 'intel_ring_reset', 'intel_ring_begin']:
    match = re.search(r'^(?:unsigned int|void|u32 \*) ?' + name + r'\(', data, re.M)
    if match is None:
        raise RuntimeError(name)
    start = data.index('{', match.start())
    depth, end = 1, start + 1
    while depth:
        depth += (data[end] == '{') - (data[end] == '}')
        end += 1
    parts.append(data[match.start():end])
out = root / 'build'
out.mkdir(exist_ok=True)
(out / 'intel_ring_slice.c').write_text(
    '// SPDX-License-Identifier: MIT\n'
    '// Copyright © 2019 Intel Corporation\n'
    '// Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/intel_ring.c\n'
    '// Ported-from-license: MIT\n'
    '#include "ring_port.h"\n\n' + '\n\n'.join(parts) + '\n')
