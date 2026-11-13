#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Reproduce whole-file failures; nonzero compiler exit remains a failure."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--linux-source', type=Path, required=True)
p.add_argument('--log', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parent
linux = a.linux_source.resolve()
revision = subprocess.check_output(['git', '-C', str(linux), 'rev-parse', 'HEAD'], text=True).strip()
if revision != 'ffc253263a1375a65fa6c9f62a893e9767fbebfa':
    p.error('expected Linux v6.6 source commit')
manifest = json.loads((root / 'build/target/manifest.json').read_text())
cmd = manifest['objects'][0]['command']
cmd = cmd[:cmd.index('-MMD')]
for directory in ['drivers/gpu/drm/i915', 'drivers/gpu/drm/i915/gt', 'include']:
    cmd += ['-I' + str(linux / directory)]
output = root / 'build/intel_ring_complete.o'
output.unlink(missing_ok=True)
cmd += ['-fmax-errors=12', '-c', str(root / 'upstream/gt/intel_ring.c'), '-o', str(output)]
with a.log.open('x') as log:
    log.write('Linux source: ' + revision + '\n' + ' '.join(cmd) + '\n')
    log.flush()
    result = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
    log.write('\ncompiler exit: %d\n' % result.returncode)
print('Whole intel_ring.c compiler exit:', result.returncode)
raise SystemExit(result.returncode)
