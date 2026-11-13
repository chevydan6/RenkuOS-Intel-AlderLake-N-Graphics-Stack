#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise native CPU adapters with target headers and modeled CPU queries."""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
m = json.loads((root/'build/target/manifest.json').read_text())
flags = m['objects'][0]['command']
flags = flags[:flags.index('-MMD')]+['-O1', '-g', '-D_KERNEL_MODE=1',
    '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
original = (root/'src/native_cpumask.c').read_text()
copy = a.output/'native_cpumask.c'
copy.write_text(original)
def build():
    objects = []
    log = ''
    for i, source in enumerate([copy, root/'src/cpumask_state.c', root/'upstream/lib/hweight.c',
                               root/'upstream/lib/find_bit.c', root/'tests/cpumask_model.c']):
        obj = a.output/(str(i)+'.o')
        command = flags.copy()
        if source.name == 'find_bit.c': command += ['-Wno-error=maybe-uninitialized']
        command += ['-c', str(source), '-o', str(obj)]
        r = subprocess.run(command, text=True, capture_output=True, timeout=30)
        log += ' '.join(command)+'\n'+r.stdout+r.stderr
        (a.output/'build.log').write_text(log)
        r.check_returncode()
        objects.append(str(obj))
    subprocess.run(['gcc', '-fsanitize=address,undefined', *objects,
        *shlex.split(os.environ.get('SANITIZER_LDFLAGS', '')), '-o', str(a.output/'test')],
        check=True, capture_output=True, timeout=30)
build()
r = subprocess.run([str(a.output/'test')], text=True, capture_output=True, timeout=30)
(a.output/'run.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
r.check_returncode()
# Treating every present CPU as online must be rejected by sparse-enabled tests.
before = 'if (info[cpu].enabled) enabled |= 1UL << cpu;'
assert original.count(before) == 1
copy.write_text(original.replace(before, 'enabled |= 1UL << cpu;'))
build()
r = subprocess.run([str(a.output/'test')], text=True, capture_output=True, timeout=30)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == 90
copy.write_text(original)
build()
print('PASS actual CPU adapter with target headers and ASan/UBSan; disabled-CPU mutation rejected')
