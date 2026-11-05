#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract exact kernel resource-init statements for a mock allocation fixture."""
import argparse
import hashlib
import pathlib
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
args = parser.parse_args()
data = args.source.read_text()
notice = re.match(r'/\*.*?\*/', data, re.S)
methods = re.findall(r'^status_t\nintel_extreme_init\(intel_info &info\)\n\{.*?^}\n',
                     data, re.M | re.S)
if notice is None or len(methods) != 1:
    raise SystemExit('expected one attributed intel_extreme_init definition')
parts = [notice.group(), '// Production SHA-256: '
         + hashlib.sha256(args.source.read_bytes()).hexdigest()]
for name, start, end, parameters in [
        ('Ring', '\t// reserve ring buffer memory', '\t// Enable clock gating', ''),
        ('Auxiliary', '\t// setup overlay registers', '\tedid1_info* edidInfo',
         ', bool hardwareCursor')]:
    body = methods[0]
    if body.count(start) != 1 or body.count(end) != 1:
        raise SystemExit(f'expected unique boundaries for {name}')
    begin, finish = body.index(start), body.index(end)
    if finish <= begin:
        raise SystemExit(f'invalid boundaries for {name}')
    parts.append(f'static void\nInitialize{name}(intel_info& info{parameters})\n{{\n'
                 + body[begin:finish] + '}\n')
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text('\n\n'.join(parts))
