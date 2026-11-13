#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
import hashlib
import io
from pathlib import Path
import subprocess
import tarfile

p = argparse.ArgumentParser()
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
root = Path(__file__).resolve().parent.parent
subprocess.run(['python3', str(root / 'verify-upstream.py')], check=True)
for script in ('extract.py', 'extract-emitter.py'):
    subprocess.run(['python3', str(root / script)], check=True)
files = [f for f in root.rglob('*') if f.is_file() and
         f.relative_to(root).parts[0] in ('include', 'src', 'upstream', 'vm')]
files += [root / name for name in ['upstream-manifest.json', 'verify-upstream.py', 'PROVENANCE.md']]
files += [root / 'build' / name for name in ('intel_ring_slice.c', 'gen8_engine_slice.c')]
manifest = ''.join(hashlib.sha256(f.read_bytes()).hexdigest() + '  ' +
                   f.relative_to(root).as_posix() + '\n' for f in sorted(files))
with a.output.open('xb') as output, tarfile.open(fileobj=output, mode='w:gz') as tar:
    for f in sorted(files):
        tar.add(f, arcname='i915-linuxkpi-native/' + f.relative_to(root).as_posix())
    data = manifest.encode()
    info = tarfile.TarInfo('i915-linuxkpi-native/manifest.sha256')
    info.size = len(data)
    tar.addfile(info, io.BytesIO(data))
print('Created', a.output, 'sha256', hashlib.sha256(a.output.read_bytes()).hexdigest())
