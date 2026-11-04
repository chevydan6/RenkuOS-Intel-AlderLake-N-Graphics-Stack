#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Export a pinned source subset and apply reviewed patches in a new build bundle."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import tarfile
import tempfile

BASE = '4799bb5a188c4bb6bbef967287640cc21a061a0d'
PATHS = [
    'License.md', 'headers/private/graphics', 'build/config_headers',
    'src/add-ons/kernel/busses/agp_gart',
    'src/add-ons/kernel/bus_managers/agp_gart',
    'src/add-ons/kernel/drivers/graphics/intel_extreme',
    'src/add-ons/kernel/drivers/graphics/common',
    'src/system/kernel/util/kernel_cpp.cpp',
    'src/add-ons/accelerants/intel_extreme', 'src/add-ons/accelerants/common',
]
PROJECT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=pathlib.Path, help='existing RenkuOS Git source; only pinned objects are exported')
parser.add_argument('--output', required=True, type=pathlib.Path, help='new bundle directory')
parser.add_argument('--unpatched', action='store_true', help='control build for regression testing')
args = parser.parse_args()
out = args.output.resolve()
if out.exists():
    parser.error('output exists; use a fresh directory to preserve prior results')
out.mkdir(parents=True)
with tempfile.TemporaryDirectory(prefix='haiku-gpu-source-') as temp:
    temp = pathlib.Path(temp)
    repo = args.source.resolve() if args.source else temp / 'haiku'
    if args.source is None:
        subprocess.run(['git', 'init', '-q', str(repo)], check=True)
        subprocess.run(['git', '-C', str(repo), 'remote', 'add', 'origin', 'https://github.com/RenkuOS/Source.git'], check=True)
        subprocess.run(['git', '-C', str(repo), 'config', 'remote.origin.promisor', 'true'], check=True)
        subprocess.run(['git', '-C', str(repo), 'config', 'remote.origin.partialclonefilter', 'blob:none'], check=True)
        subprocess.run(['git', '-C', str(repo), 'fetch', '--depth=1', '--filter=blob:none', 'origin', BASE], check=True)
    actual = subprocess.check_output(['git', '-C', str(repo), 'rev-parse', BASE + '^{commit}'], text=True).strip()
    if actual != BASE:
        raise SystemExit('source commit mismatch')
    archive = temp / 'source.tar'
    subprocess.run(['git', '-C', str(repo), 'archive', '--format=tar', '-o', str(archive), BASE, *PATHS], check=True)
    source = out / 'source'
    source.mkdir()
    with tarfile.open(archive) as tar:
        tar.extractall(source, filter='data')
    (out / 'controls').mkdir()
    shutil.copy2(source / 'src/add-ons/accelerants/intel_extreme/hooks.cpp', out / 'controls')
    shutil.copy2(source / 'src/add-ons/accelerants/intel_extreme/engine.cpp', out / 'controls')
    shutil.copy2(source / 'src/add-ons/kernel/drivers/graphics/intel_extreme/power.cpp', out / 'controls')
    shutil.copy2(source / 'src/add-ons/kernel/busses/agp_gart/intel_gart.cpp', out / 'controls')
    shutil.copy2(source / 'src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp', out / 'controls')
    # A temporary repository makes patch application relative to this source,
    # independent of the caller's working directory or enclosing repositories.
    subprocess.run(['git', 'init', '-q', str(source)], check=True)
    patches = [] if args.unpatched else sorted((PROJECT / 'patches').glob('*.patch'))
    for patch in patches:
        subprocess.run(['git', '-C', str(source), 'apply', '--check', str(patch)], check=True)
        subprocess.run(['git', '-C', str(source), 'apply', str(patch)], check=True)
    shutil.rmtree(source / '.git')
shutil.copytree(PROJECT / 'tests', out / 'tests', ignore=shutil.ignore_patterns('__pycache__'))
shutil.copytree(PROJECT / 'experimental', out / 'experimental')
for original, name in [
        (source / 'src/add-ons/kernel/bus_managers/agp_gart/agp_gart.cpp', 'current'),
        (out / 'controls/agp_gart.cpp', 'original')]:
    subprocess.run(['python3', str(PROJECT / 'tools/extract-gart-methods.py'),
                    str(original), str(out / f'tests/generated/gart-{name}.inc')], check=True)
    flags = ['--lifecycle'] + (['--teardown'] if name == 'current' else [])
    subprocess.run(['python3', str(PROJECT / 'tools/extract-gart-methods.py'), *flags,
                    str(original), str(out / f'tests/generated/gart-lifecycle-{name}.inc')],
                   check=True)
    subprocess.run(['python3', str(PROJECT / 'tools/extract-gart-methods.py'),
                    '--allocation', str(original),
                    str(out / f'tests/generated/gart-allocation-{name}.inc')],
                   check=True)
(out / 'tools').mkdir()
shutil.copy2(PROJECT / 'tools/validate-artifacts.py', out / 'tools')
shutil.copy2(PROJECT / 'tools/regression-control.py', out / 'tools')
shutil.copy2(PROJECT / 'tools/run-native.py', out / 'tools')
shutil.copy2(PROJECT / 'tools/run-native.sh', out / 'tools')
shutil.copy2(PROJECT / 'tools/Makefile.haiku', out / 'Makefile')
manifest = {
    'renkuos_commit': BASE,
    'patched': not args.unpatched,
    'patch_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in patches},
    'payload_sha256': {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in sorted(out.rglob('*')) if p.is_file()},
}
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
(out / 'manifest.sha256').write_text(''.join(
    f'{digest}  {name}\n' for name, digest in manifest['payload_sha256'].items()))
print(out)
