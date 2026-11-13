#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tarfile

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
p.add_argument('--linux-source', required=True, type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
subprocess.run(['python3', str(root/'verify-upstream.py')], check=True)
pin = json.loads((root/'upstream-manifest.json').read_text())['linuxRevision']
if subprocess.check_output(['git', '-C', str(a.linux_source), 'rev-parse', 'HEAD'], text=True).strip() != pin:
    p.error('Linux revision mismatch')
a.output.mkdir(parents=True, exist_ok=False)
for script in ('extract.py', 'extract-emitter.py'):
    subprocess.run(['python3', str(root/script)], check=True)
for name in ('include', 'src', 'upstream', 'completion', 'vma', 'vm'):
    shutil.copytree(root/name, a.output/name, ignore=shutil.ignore_patterns('__pycache__'))
(a.output/'build').mkdir()
for name in ('intel_ring_slice.c', 'gen8_engine_slice.c'):
    shutil.copyfile(root/'build'/name, a.output/'build'/name)
for name in ('upstream-manifest.json', 'verify-upstream.py', 'PROVENANCE.md'):
    shutil.copyfile(root/name, a.output/name)
# Reference headers and full i915 dependencies are byte-exact pinned files for
# genuine whole-source attempts on RenkuOS; they are not linked into the test.
lines = subprocess.check_output(['git', '-C', str(a.linux_source), 'ls-tree', '-r', pin,
                                'include', 'drivers/gpu/drm/i915'], text=True).splitlines()
count = 0
for line in lines:
    metadata, name = line.split('\t', 1)
    mode, kind, blob = metadata.split()
    if mode != '100644' or kind != 'blob': continue
    source = a.linux_source/name
    if not source.is_file(): continue
    data = source.read_bytes()
    actual = hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
    if actual != blob: raise SystemExit('Dirty reference source: '+name)
    target = a.output/'reference'/name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    count += 1
(a.output/'reference-provenance.json').write_text(json.dumps({'linuxRevision': pin,
    'regularFilesVerified': count, 'purpose': 'unchanged full-source compiler dependencies'}, indent=2)+'\n')
files = sorted(p for p in a.output.rglob('*') if p.is_file())
manifest = a.output/'payload.sha256'
manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+
                            str(p.relative_to(a.output))+'\n' for p in files))
with tarfile.open(a.output/'payload.tar.gz', 'w:gz') as tar:
    for path in files+[manifest]: tar.add(path, arcname=str(path.relative_to(a.output)))
print(a.output/'payload.tar.gz')
