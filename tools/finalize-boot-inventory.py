#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify diagnostic provenance, kernel dependencies, tests and VM cleanup."""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import tarfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
parser.add_argument('validation', type=pathlib.Path)
parser.add_argument('--transaction', required=True, type=pathlib.Path)
args = parser.parse_args()
bundle = args.bundle.resolve()
validation = args.validation.resolve()
manifest_path = bundle / 'manifest.json'
manifest = json.loads(manifest_path.read_text())
if manifest.get('kind') != 'native-boot-graphics-inventory':
    raise SystemExit('unexpected bundle kind')
if (validation / 'exit-status').read_text().strip() != '0':
    raise SystemExit('native validation failed')
if (validation / 'cleanup.log').read_text().strip() != 'cleanup_status=0':
    raise SystemExit('guest diagnostic cleanup failed')
transaction = json.loads(args.transaction.read_text())
if transaction != {'component_exit_status': 0, 'shutdown': 'guest shutdown',
                   'gpu_execution': False, 'vm_exit_status': 0}:
    raise SystemExit('VM did not finish and shut down cleanly')
expected = {name + ': OK' for name in manifest['payload_sha256']}
actual = (validation / 'manifest-check.log').read_text().splitlines()
if set(actual) != expected or len(actual) != len(expected):
    raise SystemExit('guest payload coverage mismatch')
for name, digest in manifest['payload_sha256'].items():
    if hashlib.sha256((bundle / name).read_bytes()).hexdigest() != digest:
        raise SystemExit('local payload changed: ' + name)
log = (validation / 'build-test.log').read_text().splitlines()
for required in ('34 boot framebuffer checks, 0 failures',
                 '82 native boot diagnostic checks, 0 failures'):
    if log.count(required) != 1:
        raise SystemExit('missing or repeated native test result: ' + required)
if any(line.startswith('FAIL ') for line in log):
    raise SystemExit('native test failure present')
artifact_dir = validation / 'artifacts'
artifact_dir.mkdir(exist_ok=False)
names = {'boot-graphics-inventory', 'boot_graphics',
         'boot-framebuffer-info-test', 'boot-graphics-driver-test'}
with tarfile.open(validation / 'artifacts.tar.gz') as tar:
    members = tar.getmembers()
    if {item.name for item in members} != names or len(members) != len(names) \
            or not all(item.isfile() for item in members):
        raise SystemExit('unexpected artifact members')
    tar.extractall(artifact_dir, filter='data')
artifacts = {}
for name in sorted(names):
    path = artifact_dir / name
    header = subprocess.check_output(['readelf', '-h', str(path)], text=True)
    if 'ELF64' not in header or 'Advanced Micro Devices X86-64' not in header:
        raise SystemExit('wrong artifact architecture: ' + name)
    artifacts[name] = {'bytes': path.stat().st_size,
                      'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
for line in (validation / 'artifact.sha256').read_text().splitlines():
    digest, name = line.split()
    if name not in artifacts or artifacts[name]['sha256'] != digest:
        raise SystemExit('guest artifact hash mismatch')
hash_names = [line.split()[1] for line in
              (validation / 'artifact.sha256').read_text().splitlines()]
if sorted(hash_names) != ['boot-graphics-inventory', 'boot_graphics']:
    raise SystemExit('missing or repeated guest artifact hash')
driver = artifact_dir / 'boot_graphics'
dynamic = subprocess.check_output(['readelf', '-d', str(driver)], text=True)
if re.findall(r'\(NEEDED\).*\[(.*?)\]', dynamic) != ['_KERNEL_']:
    raise SystemExit('unexpected diagnostic kernel dependencies')
symbols = subprocess.check_output(['nm', '-D', '--defined-only', str(driver)], text=True)
for symbol in ('init_hardware', 'init_driver', 'uninit_driver',
               'publish_devices', 'find_device', 'api_version'):
    if not re.search(r'\b' + symbol + r'$', symbols, re.M):
        raise SystemExit('missing driver symbol: ' + symbol)
record = {
    'source_commit': manifest['renkuos_commit'],
    'source_manifest_sha256': hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
    'payload_files': len(expected),
    'uname': (validation / 'kernel.txt').read_text().strip(),
    'compiler': (validation / 'compiler.txt').read_text().splitlines()[0],
    'artifacts': artifacts,
    'diagnostic_kernel_execution': True,
    'intel_driver_execution': False,
    'gpu_execution': False,
    'transaction': transaction,
}
(validation / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
print(json.dumps(record, indent=2))
