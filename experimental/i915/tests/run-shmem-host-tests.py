#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
from pathlib import Path
import subprocess
import sys
import resource
import json

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
flags = ['gcc', '-D_GNU_SOURCE', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra',
         '-Werror', '-Wno-unused-parameter', '-Wno-sign-compare', '-fno-strict-overflow',
         '-pthread', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         '-fno-omit-frame-pointer', '-no-pie', '-I'+str(root/'include'),
         '-I'+str(root/'upstream')]
units = ['src/native_pages.c', 'src/task_identity.c', 'upstream/lib/refcount.c',
         'completion/host_backend.c', 'tests/page_bridge_host.c',
         'tests/shmem_ownership_test.c']
if (root/'src/native_shmem.c').is_file():
    units.append('src/native_shmem.c')
originals = {name: (root/name).read_text() for name in ('src/native_pages.c', 'src/native_shmem.c')
             if (root/name).is_file()}
copies = {}
for name, text in originals.items():
    copies[name] = a.output/Path(name).name
    copies[name].write_text(text)
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
def run(label):
    command = flags + [str(copies.get(x, root/x)) for x in units] + ['-o', str(a.output/'test')]
    r = subprocess.run(command, capture_output=True, text=True)
    (a.output/(label+'-build.log')).write_text(' '.join(command)+'\n'+r.stdout+r.stderr)
    if r.returncode:
        print(r.stdout+r.stderr, file=sys.stderr)
        raise SystemExit(r.returncode)
    r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=20)
    (a.output/(label+'.log')).write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
    return r
r = run('positive')
print(r.stdout+r.stderr, end='')
if r.returncode:
    raise SystemExit(r.returncode)
mutations = [
    ('close-ignores-pins', 'src/native_pages.c',
     '!store->closing || store->pins || store->in_flight || store->queued',
     '!store->closing || store->in_flight || store->queued'),
    ('close-ignores-inflight', 'src/native_pages.c',
     '!store->closing || store->pins || store->in_flight || store->queued',
     '!store->closing || store->pins || store->queued'),
    ('noreserve-lost', 'src/native_pages.c',
     'renku_vm_page_store_create_policy(size, noreserve, &store->base)',
     'renku_vm_page_store_create_policy(size, false, &store->base)'),
    ('callback-twice', 'src/native_pages.c',
     'if (released) released(cookie);',
     'if (released) { released(cookie); released(cookie); }'),
    ('file-reference-lost', 'src/native_shmem.c',
     'BUG_ON(!file || !refcount_inc_not_zero(&file->renku_references));', 'BUG_ON(!file);'),
    ('gfp-constraint-ignored', 'src/native_shmem.c',
     'if (!preemptible() || gfp != GFP_KERNEL)', 'if (!preemptible())'),
]
controls = []
for label, name, before, after in mutations:
    if name not in originals:
        continue
    assert before in originals[name], label
    copies[name].write_text(originals[name].replace(before, after, 1))
    r = run(label)
    assert r.returncode != 0 and any(x in r.stderr for x in
        ('Assertion', 'assertion', 'AddressSanitizer', 'runtime error:')), (label, r.stdout, r.stderr)
    controls.append({'name': label, 'exit': r.returncode})
    copies[name].write_text(originals[name])
r = run('restored')
assert r.returncode == 0, (r.stdout, r.stderr)
(a.output/'mutation-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
print('PASS native shmem ownership and',len(controls),'rejected lifetime/policy mutations under ASan/UBSan')
