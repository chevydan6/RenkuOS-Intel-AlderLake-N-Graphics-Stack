#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tarfile

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
subprocess.run(['python3', str(root/'verify-upstream.py')], check=True)
subprocess.run(['python3', str(root/'extract-gfp.py')], check=True)
a.output.mkdir(parents=True, exist_ok=False)
for name in ('include', 'src', 'upstream', 'vma'):
    shutil.copytree(root/name, a.output/name, ignore=shutil.ignore_patterns('__pycache__'))
(a.output/'build').mkdir()
for name in ('gfp_helpers.h', 'gfp_helpers.json'):
    shutil.copyfile(root/'build'/name, a.output/'build'/name)
for name in ('upstream-manifest.json', 'verify-upstream.py', 'PROVENANCE.md'):
    shutil.copyfile(root/name, a.output/name)
files = sorted(p for p in a.output.rglob('*') if p.is_file())
manifest = a.output/'payload.sha256'
manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+
                            str(p.relative_to(a.output))+'\n' for p in files))
with tarfile.open(a.output/'payload.tar.gz', 'w:gz') as tar:
    for path in files+[manifest]:
        tar.add(path, arcname=str(path.relative_to(a.output)))
print(a.output/'payload.tar.gz')
