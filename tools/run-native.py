#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run isolated native component checks and record provenance; never install."""
import argparse
import datetime
import hashlib
import json
import pathlib
import platform
import subprocess
import sys
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
parser.add_argument('--previous', type=pathlib.Path, help='previous result.json for regression status')
args = parser.parse_args()
bundle = args.bundle.resolve()
if platform.system() != 'Haiku' or platform.machine() != 'x86_64':
    parser.error('native x86_64 Haiku/RenkuOS userspace required')
manifest = json.loads((bundle / 'manifest.json').read_text())
for relative, expected in manifest['payload_sha256'].items():
    if hashlib.sha256((bundle / relative).read_bytes()).hexdigest() != expected:
        raise SystemExit(f'payload changed: {relative}; create a fresh bundle')
record = bundle / 'validation'
record.mkdir(exist_ok=False)
info = {
    'timestamp_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'uname': subprocess.check_output(['uname', '-a'], text=True).strip(),
    'compiler': subprocess.check_output(['g++', '--version'], text=True).splitlines()[0],
    'source_manifest_sha256': hashlib.sha256((bundle / 'manifest.json').read_bytes()).hexdigest(),
    'source_commit': manifest['renkuos_commit'],
    'patched': manifest['patched'],
    'runtime_validation': 'user-space hooks, GGTT helpers and simulated-register lifecycle; kernel module not loaded',
    'gpu_execution': False,
}
identity = pathlib.Path('/boot/system/data/renkuos/build')
info['renkuos_build'] = identity.read_text().strip() if identity.is_file() else None
start = time.monotonic()
with (record / 'build-test.log').open('w') as log:
    result = subprocess.run(['make', '-j1', 'test'], cwd=bundle, stdout=log, stderr=subprocess.STDOUT)
info['elapsed_seconds'] = round(time.monotonic() - start, 3)
info['exit_status'] = result.returncode
info['artifacts'] = {}
for name in ['intel_gart', 'agp_gart', 'intel_extreme.driver', 'intel_extreme.accelerant', 'hooks-test', 'gtt-range-test', 'gtt-pte-test', 'gtt-stolen-test', 'lifecycle-test', 'gating-test', 'gart-range-test', 'gart-lifecycle-test']:
    p = bundle / 'build' / name
    if p.is_file():
        info['artifacts'][name] = {'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
if args.previous:
    previous = json.loads(args.previous.read_text())
    info['previous_source_commit'] = previous.get('source_commit')
    info['test_status_regression'] = previous.get('exit_status') == 0 and result.returncode != 0
(record / 'result.json').write_text(json.dumps(info, indent=2) + '\n')
print(json.dumps(info, indent=2))
raise SystemExit(result.returncode)
