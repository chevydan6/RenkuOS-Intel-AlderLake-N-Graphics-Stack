#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export pinned native baseline + 0001..0096, then isolated mapping candidates."""
import argparse, ast, hashlib, json, pathlib, shutil, subprocess, tarfile
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
a = p.parse_args()
project = pathlib.Path(__file__).resolve().parents[3]
constants = {}
for node in ast.parse((project/'tools/prepare.py').read_text()).body:
    if isinstance(node, ast.Assign) and isinstance(node.value, (ast.Constant, ast.List)):
        constants[node.targets[0].id] = ast.literal_eval(node.value)
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
source = out/'source'
source.mkdir()
archive = out/'baseline.tar'
def run(cmd):
    subprocess.run(cmd, check=True)
run(['git','-C',str(a.source),'archive','--format=tar','-o',str(archive),constants['BASE'],*constants['PATHS']])
with tarfile.open(archive) as tar:
    tar.extractall(source, filter='data')
archive.unlink()
run(['git','init','-q',str(source)])
patches = [p for p in sorted((project/'patches').glob('*.patch')) if int(p.name[:4]) <= 96]
patches += [pathlib.Path(__file__).with_name(n) for n in ('gart-invalidation.patch','owner-mapping.patch')]
record = {'baseline':constants['BASE'], 'patches':{}}
for patch in patches:
    run(['git','-C',str(source),'apply','--check',str(patch)])
    run(['git','-C',str(source),'apply',str(patch)])
    record['patches'][patch.name] = hashlib.sha256(patch.read_bytes()).hexdigest()
shutil.rmtree(source/'.git')
shutil.copytree(pathlib.Path(__file__).parent, out/'mapping', ignore=shutil.ignore_patterns('__pycache__'))
(out/'provenance.json').write_text(json.dumps(record, indent=2)+'\n')
files = [p for d in (source,out/'mapping') for p in d.rglob('*') if p.is_file()]+[out/'provenance.json']
manifest = out/'payload.sha256'
manifest.write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p.relative_to(out))+'\n' for p in sorted(files)))
with tarfile.open(out/'payload.tar.gz','w:gz') as tar:
    for file in files+[manifest]:
        tar.add(file, arcname=str(file.relative_to(out)))
print(out/'payload.tar.gz')
