#!/usr/bin/env python3
"""Decode the pinned Linux table, independently of the native constructor."""
import argparse
import hashlib
from pathlib import Path
import re

p = argparse.ArgumentParser()
p.add_argument('linux', type=Path)
p.add_argument('--write', action='store_true')
a = p.parse_args()
source = a.linux / 'drivers/gpu/drm/i915/gt/intel_lrc.c'
s = source.read_bytes()
cs = a.linux / 'drivers/gpu/drm/i915/gt/gen8_engine_cs.c'
assert hashlib.sha256(cs.read_bytes()).hexdigest() == '9583d998fd94e7bf6c3dd7e17637a3c88cf315c5bba1f2a30bda6ccefd3c9d18', 'gen8_engine_cs.c source identity differs'
# Linux v6.6 source identity: fail closed on source drift.
PIN = '05867c37aa7cef99a3c636c14761ea31378a9408af7c882c235461b56e81fcb4'
assert hashlib.sha256(s).hexdigest() == PIN, 'intel_lrc.c source identity differs'
body = re.search(rb'static const u8 gen12_rcs_offsets\[\] = \{(.*?)\n\};', s, re.S)[1].decode()
words = [0] * 1024
pos = 0
for op, expr in re.findall(r'(NOP|LRI|REG16|REG)\(([^)]*)\)', body):
    if op == 'NOP':
        pos += sum(int(x.strip(), 0) for x in expr.split('+'))
    elif op == 'LRI':
        count, flag = expr.split(',')
        words[pos] = 0x11080000 | ((int(count) * 2) - 1) | (0x1000 if flag.strip() == 'POSTED' else 0)
        pos += 1
    else:
        words[pos] = 0x2000 + int(expr, 0)
        pos += 2
assert pos == 0xb9
words[pos] = 0x05000001
text = '// SPDX-License-Identifier: MIT\n// Copyright © 2014 Intel Corporation\n// Generated from Linux v6.6 intel_lrc.c by check-rcs-reference.py.\n'
text += 'static const uint32_t kLinuxRcsRegisters[1024] = {\n'
for i in range(0, pos + 1, 6):
    text += '\t' + ', '.join(f'0x{x:08x}' for x in words[i:min(i+6, pos+1)]) + ',\n'
text += '};\n'
out = Path(__file__).with_name('rcs_linux_golden.h')
if a.write:
    out.write_text(text)
else:
    assert out.read_text() == text, 'golden differs; inspect source before regenerating'
print('Linux v6.6 exact RCS register reference: PASS')
