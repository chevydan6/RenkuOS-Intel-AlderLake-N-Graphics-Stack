#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Keep original batch-start and ring-advance bodies; adapt object interfaces."""
from pathlib import Path
import re

root = Path(__file__).resolve().parent

def function(path, pattern):
    text = (root / path).read_text()
    match = re.search(pattern, text, re.M)
    if match is None:
        raise RuntimeError(pattern)
    pos = text.index('{', match.start())
    depth, end = 1, pos + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]

parts = [function('upstream/gt/intel_ring.h', r'^static inline void intel_ring_advance\(')]
for name in ['gen8_emit_bb_start_noarb', 'gen8_emit_bb_start']:
    parts.append(function('upstream/gt/gen8_engine_cs.c', r'^int ' + name + r'\('))
out = root / 'build'
out.mkdir(exist_ok=True)
(out / 'gen8_engine_slice.c').write_text(
    '// SPDX-License-Identifier: MIT\n'
    '// Copyright © 2014, 2019 Intel Corporation\n'
    '// Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.c\n'
    '// Ported-from-license: MIT\n'
    '// Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/intel_ring.h\n'
    '// Ported-from-license: MIT\n'
    '#include "emitter_port.h"\n\n' + '\n\n'.join(parts) + '\n')
