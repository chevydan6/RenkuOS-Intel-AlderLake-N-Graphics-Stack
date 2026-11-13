#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export unchanged native bus_dma sources and the Linux DMA compatibility test."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import tarfile

BASE = '4799bb5a188c4bb6bbef967287640cc21a061a0d'
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
subprocess.run(['python3', str(root/'dma/extract-scatterlist.py')], check=True)
subprocess.run(['python3', str(root/'extract-folio.py')], check=True)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
source = out/'source'
source.mkdir()
archive = out/'baseline.tar'
subprocess.run(['git', '-C', str(a.source), 'archive', '--format=tar', '-o', str(archive),
                BASE, 'src/libs/compat/freebsd_network', 'build/config_headers'], check=True)
with tarfile.open(archive) as tar:
    tar.extractall(source, filter='data')
archive.unlink()
for name in ('include', 'upstream', 'dma'):
    shutil.copytree(root/name, out/'port'/name, ignore=shutil.ignore_patterns('__pycache__'))
for name in ('src/native_services.c', 'src/native_diagnostics.c', 'build/scatterlist_slice.c', 'build/folio_helpers.h', 'build/folio_helpers.json'):
    target = out/'port'/name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(root/name, target)
shutil.copyfile(root/'dma/build-native.sh', out/'build-native.sh')
shutil.copyfile(root/'dma/run-native-tests.sh', out/'run-native-tests.sh')
for name in ('upstream-manifest.json', 'PROVENANCE.md'):
    shutil.copyfile(root/name, out/'port'/name)
(out/'provenance.json').write_text(json.dumps({
    'renkuRevision': BASE,
    'linuxRevision': json.loads((root/'upstream-manifest.json').read_text())['linuxRevision'],
    'nativeSourcesModified': False,
    'scope': 'Linux DMA API compatibility; no PCI attachment or GPU submission',
}, indent=2)+'\n')
files = sorted(path for path in out.rglob('*') if path.is_file())
manifest = out/'payload.sha256'
manifest.write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+
                            str(path.relative_to(out))+'\n' for path in files))
with tarfile.open(out/'payload.tar.gz', 'w:gz') as tar:
    for path in files+[manifest]:
        tar.add(path, arcname=str(path.relative_to(out)))
print(out/'payload.tar.gz')
