#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run the complete original DRM VMA manager under ASan/UBSan."""
import argparse
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
subprocess.run(['python3', str(root/'verify-upstream.py')], check=True)
original = (root/'upstream/drm/drm_vma_manager.c').read_text()
source = a.output/'drm_vma_manager.c'
source.write_text(original)
command = ['gcc', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
           '-Wno-unused-parameter', '-Wno-sign-compare', '-fno-strict-overflow',
           '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-pthread',
           '-I'+str(root/'include'), '-I'+str(root/'upstream'), '-I'+str(root/'vma'),
           str(source), str(root/'upstream/drm/drm_mm.c'), str(root/'upstream/lib/rbtree.c'),
           str(root/'vma/host_test.c'), *shlex.split(os.environ.get('SANITIZER_LDFLAGS', '')),
           '-o', str(a.output/'test')]
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
for name, cmd in [('build', command), ('run', [str(a.output/'test')])]:
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
    (a.output/(name+'.log')).write_text(r.stdout+r.stderr)
    r.check_returncode()
# Preserve upstream files: change only the exclusive test copy for this control.
before = 'new->vm_count = 1;'
assert original.count(before) == 1
source.write_text(original.replace(before, 'new->vm_count = 2;'))
subprocess.run(command, check=True, capture_output=True, timeout=60)
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=60)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
source.write_text(original)
subprocess.run(command, check=True, capture_output=True, timeout=60)
print('PASS whole DRM VMA manager and original log2 helpers; permission regression rejected')
