#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Validate actual native RCU glue with target headers and explicit OS models."""
import argparse
import json
import os
from pathlib import Path
import resource
import shlex
import subprocess
p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
m = json.loads((root/'build/target/manifest.json').read_text())
flags = m['objects'][0]['command']; flags = flags[:flags.index('-MMD')]
flags += ['-O1', '-g', '-D_KERNEL_MODE=1', '-ffunction-sections', '-fdata-sections',
          '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
original = (root/'src/native_rcu.c').read_text()
copy = a.output/'native_rcu.c'; copy.write_text(original)
def build():
    objects = []
    log = ''
    units = [copy, root/'src/native_irq_scopes.c', root/'src/native_rwlocks.c',
             root/'src/native_preempt.c', root/'tests/rcu_model.c', root/'tests/rcu_model_host.c']
    for i, source in enumerate(units):
        obj = a.output/(str(i)+'.o')
        command = flags.copy() if i < 5 else ['gcc', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra',
            '-Werror', '-pthread', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        command += ['-c', str(source), '-o', str(obj)]
        r = subprocess.run(command, text=True, capture_output=True, timeout=30)
        log += ' '.join(command)+'\n'+r.stdout+r.stderr
        (a.output/'build.log').write_text(log); r.check_returncode(); objects.append(str(obj))
    r = subprocess.run(['gcc', '-pthread', '-fsanitize=address,undefined', '-Wl,--gc-sections', *objects,
        *shlex.split(os.environ.get('SANITIZER_LDFLAGS', '')), '-o', str(a.output/'test')],
        capture_output=True, text=True, timeout=30)
    (a.output/'link.log').write_text(r.stdout+r.stderr); r.check_returncode()
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
build()
r = subprocess.run([str(a.output/'test')], text=True, capture_output=True, timeout=15)
(a.output/'run.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr); r.check_returncode()
r = subprocess.run([str(a.output/'test'), 'invalid-reader-sync'], text=True, capture_output=True, timeout=15)
(a.output/'context-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -4
r = subprocess.run([str(a.output/'test'), 'entry-under-atomic'], text=True, capture_output=True, timeout=15)
(a.output/'entry-context-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -4
before = '\twrite_lock(&sReaders);\n\twrite_unlock(&sReaders);'
assert original.count(before) == 1
copy.write_text(original.replace(before, '\t__atomic_thread_fence(__ATOMIC_SEQ_CST);'))
build()
r = subprocess.run([str(a.output/'test')], text=True, capture_output=True, timeout=15)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == 90
copy.write_text(original); build()
print('PASS actual RCU adapter with target headers and modeled OS locks; omitted grace wait rejected')
