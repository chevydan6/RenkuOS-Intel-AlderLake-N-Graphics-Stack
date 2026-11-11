#!/usr/bin/env python3
"""Offline pinned source/ordered-patch evidence; never builds or touches hardware."""
import argparse, ast, hashlib, json, os, pathlib, subprocess, tarfile
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
a = p.parse_args()
root = pathlib.Path(__file__).resolve().parents[1]
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)
env = dict(os.environ, GIT_NO_LAZY_FETCH='1', GIT_TERMINAL_PROMPT='0')
record = {'commands': [], 'patches': [], 'source_repository': str(a.source.resolve()), 'complete': False}
def save():
    (out / 'manifest.json').write_text(json.dumps(record, indent=2) + '\n')
def run(argv):
    r = subprocess.run(argv, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    record['commands'].append({'argv': argv, 'exit': r.returncode, 'stdout': r.stdout, 'stderr': r.stderr})
    save()
    if r.returncode:
        raise SystemExit('Stopped: ' + ' '.join(argv) + '\n' + r.stderr)
    return r.stdout.strip()
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
# Read constants without importing prepare.py (which performs preparation at import).
constants = {}
for node in ast.parse((root / 'tools/prepare.py').read_text()).body:
    if isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Name) and node.targets[0].id in ('BASE', 'PATHS'):
        constants[node.targets[0].id] = ast.literal_eval(node.value)
base = constants['BASE']
record['revision'] = run(['git', '-C', str(a.source), 'rev-parse', base + '^{commit}'])
if record['revision'] != base:
    raise SystemExit('Pinned revision mismatch')
record['object_type'] = run(['git', '-C', str(a.source), 'cat-file', '-t', base])
kernel = ['src/system/kernel/module.cpp', 'headers/os/drivers/module.h', 'headers/private/kernel/lock.h']
archive = out / 'pinned.tar'
run(['git', '-C', str(a.source), 'archive', '--format=tar', '-o', str(archive), base, *constants['PATHS'], *kernel])
record['archive_sha256'] = sha(archive)
source = out / 'source'
source.mkdir()
with tarfile.open(archive) as t:
    t.extractall(source, filter='data')
record['pinned_kernel_sha256'] = {name: sha(source / name) for name in kernel}
run(['git', 'init', '-q', str(source)])
patches = sorted((root / 'patches').glob('*.patch'))
record['patch_inventory'] = [x.name for x in patches]
for i, patch in enumerate(patches, 1):
    item = {'order': i, 'name': patch.name, 'sha256': sha(patch), 'checked': False, 'applied': False}
    record['patches'].append(item)
    run(['git', '-C', str(source), 'apply', '--check', str(patch)])
    item['checked'] = True
    run(['git', '-C', str(source), 'apply', str(patch)])
    item['applied'] = True
files = kernel + ['src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp', 'headers/private/graphics/intel_extreme/IntelMemoryOwner.h', 'src/add-ons/kernel/drivers/graphics/intel_extreme/intel_extreme.cpp']
record['final_sha256'] = {name: sha(source / name) for name in files}
record['complete'] = True
save()
print(str(out / 'manifest.json'))
