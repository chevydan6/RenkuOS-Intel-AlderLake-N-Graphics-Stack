#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
import os
from pathlib import Path
import resource
import json
import shlex
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
subprocess.run(['python3', str(root/'verify-upstream.py')], check=True)
original = (root/'upstream/kernel/sched/completion.c').read_text()
copy = a.output/'completion.c'
copy.write_text(original)
flags = ['gcc', '-D_GNU_SOURCE', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-Wno-unused-parameter', '-Wno-sign-compare', '-fno-strict-overflow', '-pthread',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         '-I'+str(root/'include'), '-I'+str(root/'upstream'), '-I'+str(root/'completion')]
objects = []
def build():
    units = [copy, root/'upstream/kernel/sched/swait.c', root/'src/completion_sched.c',
             root/'src/task_identity.c', root/'src/time.c', root/'completion/host_backend.c',
             root/'completion/host_test.c']
    log = ''
    for i, source in enumerate(units):
        obj = a.output/(str(i)+'.o')
        command = flags.copy()
        if i < 2: command += ['-include', str(root/'include/completion_port.h')]
        command += ['-c', str(source), '-o', str(obj)]
        r = subprocess.run(command, capture_output=True, text=True, timeout=30)
        log += ' '.join(command)+'\n'+r.stdout+r.stderr
        (a.output/'build.log').write_text(log)
        r.check_returncode()
        if obj not in objects: objects.append(obj)
    subprocess.run(flags+list(map(str, objects))+shlex.split(os.environ.get('SANITIZER_LDFLAGS', ''))+
                   ['-o', str(a.output/'test')], check=True, capture_output=True, timeout=30)
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'run.log').write_text(r.stdout+r.stderr)
r.check_returncode()
# The canonical helpers must still reject broken assumptions and mixed types.
controls = []
for name, body, should_pass in [
    ('helper_positive', 'BUILD_BUG_ON(sizeof(unsigned long) != 8); return min(3, 7);', True),
    ('build_bug_rejected', 'BUILD_BUG_ON(1); return 0;', False),
    ('mixed_min_rejected', 'int a = 1; unsigned long b = 2; return min(a, b);', False),
]:
    source = a.output/(name+'.c')
    source.write_text('#include <linux/minmax.h>\n#include <linux/build_bug.h>\nint check(void) {'+body+'}\n')
    cmd = flags+['-c', str(source), '-o', str(a.output/(name+'.o'))]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text(result.stdout+result.stderr)
    assert (result.returncode == 0) == should_pass
    if not should_pass: assert 'error:' in result.stderr
    controls.append({'name': name, 'exit': result.returncode})
(a.output/'helper-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
before = 'x->done--;'
assert original.count(before) == 2
copy.write_text(original.replace(before, 'x->done = 0;'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
copy.write_text(original)
build()
print('PASS original completion/swait under ASan/UBSan; lost completion-count regression rejected')
