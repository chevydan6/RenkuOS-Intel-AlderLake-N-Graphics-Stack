#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Package the native inventory diagnostic with pinned RenkuOS interface headers."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import tarfile
import tempfile

BASE = '4799bb5a188c4bb6bbef967287640cc21a061a0d'
PROJECT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', required=True, type=pathlib.Path)
parser.add_argument('--output', required=True, type=pathlib.Path)
args = parser.parse_args()
source = args.source.resolve()
out = args.output.resolve()
if out.exists():
    parser.error('output exists; preserve earlier evidence with a new directory')
actual = subprocess.check_output(['git', '-C', str(source), 'rev-parse',
                                  BASE + '^{commit}'], text=True).strip()
if actual != BASE:
    raise SystemExit('source commit mismatch')
out.mkdir(parents=True)
with tempfile.TemporaryDirectory(prefix='gpu-inventory-') as temp:
    archive = pathlib.Path(temp) / 'headers.tar'
    subprocess.run(['git', '-C', str(source), 'archive', '--format=tar',
                    '-o', str(archive), BASE, 'License.md',
                    'headers/private/drivers/poke.h',
                    'headers/private/graphics/vesa/vesa_info.h',
                    'headers/private/graphics/common'], check=True)
    with tarfile.open(archive) as tar:
        tar.extractall(out / 'source', filter='data')
shutil.copy2(PROJECT / 'diagnostics/boot-graphics-inventory.cpp', out)
(out / 'tools').mkdir()
shutil.copy2(PROJECT / 'diagnostics/run-native.sh', out / 'tools')
manifest = {
    'renkuos_commit': BASE,
    'kind': 'native-boot-graphics-inventory',
    'driver_installation': False,
    'gpu_execution': False,
    'payload_sha256': {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in sorted(out.rglob('*')) if p.is_file()},
}
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
(out / 'manifest.sha256').write_text(''.join(
    f'{digest}  {name}\n' for name, digest in manifest['payload_sha256'].items()))
print(out)
