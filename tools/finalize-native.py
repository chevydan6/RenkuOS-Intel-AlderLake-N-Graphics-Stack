#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Host-side provenance/artifact checks after a shell-only native test run."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import sys
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
parser.add_argument('validation', type=pathlib.Path)
args = parser.parse_args()
bundle = args.bundle.resolve()
validation = args.validation.resolve()
subprocess.run([sys.executable, str(pathlib.Path(__file__).with_name('check-source.py')),
                str(bundle)], check=True)
status = int((validation / 'exit-status').read_text())
manifest_path = bundle / 'manifest.json'
manifest = json.loads(manifest_path.read_text())
record = {
    'source_commit': manifest['renkuos_commit'],
    'source_manifest_sha256': hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
    'patch_sha256': manifest['patch_sha256'],
    'uname': (validation / 'kernel.txt').read_text().strip(),
    'compiler': (validation / 'compiler.txt').read_text().splitlines()[0],
    'devel_package': (validation / 'devel-package.txt').read_text().strip(),
    'exit_status': status,
    'gpu_execution': False,
    'runtime_validation': 'native hooks, GGTT helpers and simulated-register lifecycle; no module load',
    'artifacts': {},
}
if status == 0:
    hashes = (validation / 'manifest-check.log').read_text().splitlines()
    expected = {name + ': OK' for name in manifest['payload_sha256']}
    if set(hashes) != expected or len(hashes) != len(expected):
        raise SystemExit('guest payload verification did not cover the entire bundle')
    artifact_dir = validation / 'artifacts'
    artifact_dir.mkdir(exist_ok=False)
    with tarfile.open(validation / 'artifacts.tar.gz') as tar:
        tar.extractall(artifact_dir, filter='data')
    subprocess.run([sys.executable, str(pathlib.Path(__file__).with_name('validate-artifacts.py')),
                    str(artifact_dir / 'build')] + (['--require-memory-owner'] if
                    (bundle / 'source/src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp').exists()
                    else []), check=True)
    for p in sorted((artifact_dir / 'build').iterdir()):
        record['artifacts'][p.name] = {
            'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
(validation / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
raise SystemExit(status)
