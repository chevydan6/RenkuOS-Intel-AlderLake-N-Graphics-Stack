#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Execute actual native adapter code with a serialized native-API model."""
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
manifest = json.loads((root/'build/target/manifest.json').read_text())
command = manifest['objects'][0]['command']
flags = command[:command.index('-MMD')]
flags += ['-O1', '-g', '-D_KERNEL_MODE=1', '-ffunction-sections', '-fdata-sections',
          '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
original = (root/'src/native_irq_scopes.c').read_text()
copy = a.output/'native_irq_scopes.c'
copy.write_text(original)
def build():
    objects = []
    log = ''
    for i, source in enumerate([copy, root/'src/native_locks.c', root/'src/native_rwlocks.c',
                               root/'src/native_preempt.c', root/'tests/irq_scope_model.c']):
        obj = a.output/(str(i)+'.o')
        cmd = flags+['-c', str(source), '-o', str(obj)]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        log += ' '.join(cmd)+'\n'+r.stdout+r.stderr
        (a.output/'build.log').write_text(log)
        r.check_returncode()
        objects.append(str(obj))
    subprocess.run(['gcc', '-fsanitize=address,undefined', '-Wl,--gc-sections', *objects,
                    *shlex.split(os.environ.get('SANITIZER_LDFLAGS', '')), '-o', str(a.output/'test')],
                   check=True, capture_output=True, timeout=30)
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'run.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
r.check_returncode()
before = 'if (scope->masks) scope->requested = saved != 0;'
assert original.count(before) == 1
copy.write_text(original.replace(before, 'if (scope->masks) { scope->requested = saved != 0; restore_interrupts(saved); }'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == 90
copy.write_text(original)
build()
print('PASS actual native IRQ/preemption adapter under ASan/UBSan with modeled OS services; premature enable rejected')
