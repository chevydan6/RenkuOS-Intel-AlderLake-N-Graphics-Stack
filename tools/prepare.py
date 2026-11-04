#!/usr/bin/env python3
"""Export a pinned source subset and apply reviewed patches in a new build bundle."""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
import tarfile
import tempfile

BASE = '4f1074d5e14f59e0f5e5c4938c8a50d1290fa59c'
PATHS = [
    'License.md', 'headers/private/graphics',
    'src/add-ons/kernel/busses/agp_gart',
    'src/add-ons/accelerants/intel_extreme', 'src/add-ons/accelerants/common',
]
PROJECT = pathlib.Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', type=pathlib.Path, help='existing Haiku Git source; only pinned objects are exported')
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
        subprocess.run(['git', '-C', str(repo), 'remote', 'add', 'origin', 'https://github.com/haiku/haiku.git'], check=True)
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
    # A temporary repository makes patch application relative to this source,
    # independent of the caller's working directory or enclosing repositories.
    subprocess.run(['git', 'init', '-q', str(source)], check=True)
    patches = [] if args.unpatched else sorted((PROJECT / 'patches').glob('*.patch'))
    for patch in patches:
        subprocess.run(['git', '-C', str(source), 'apply', '--check', str(patch)], check=True)
        subprocess.run(['git', '-C', str(source), 'apply', str(patch)], check=True)
    shutil.rmtree(source / '.git')
shutil.copytree(PROJECT / 'tests', out / 'tests', ignore=shutil.ignore_patterns('__pycache__'))
(out / 'tools').mkdir()
shutil.copy2(PROJECT / 'tools/validate-artifacts.py', out / 'tools')
shutil.copy2(PROJECT / 'tools/Makefile.haiku', out / 'Makefile')
manifest = {
    'haiku_commit': BASE,
    'patched': not args.unpatched,
    'patch_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in patches},
    'payload_sha256': {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in sorted(out.rglob('*')) if p.is_file()},
}
(out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(out)
