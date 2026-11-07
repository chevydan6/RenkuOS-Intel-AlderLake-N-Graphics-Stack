#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise unchanged production observation functions with threaded PCI mocks."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
source = a.source / 'src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp'
text = source.read_text()
globals_start = text.index('struct OwnerDevice {')
globals_end = text.index('static status_t\nStatus(')
status_end = text.index('// A ticket is ended', globals_end)
functions_start = text.index('static bool\nReadConfig(')
functions_end = text.index('// v1 acquires v0', functions_start)
(a.output / 'observation-under-test.inc').write_text(
    text[globals_start:status_end] + text[functions_start:functions_end])
headers = a.source / 'headers/private/graphics/intel_extreme'
inputs = [source]
for name in ('IntelApertureLeaseState.h', 'IntelN100PciPower.h', 'IntelN100GtAck.h', 'IntelN100Forcewake.h'):
    shutil.copy2(headers / name, a.output / name)
    inputs.append(headers / name)
test = Path(__file__).with_name('owner-observation-test.cpp')
shutil.copy2(test, a.output / test.name)
inputs.append(test)
command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pthread',
           '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
           '-I', str(a.output), str(a.output / test.name), '-o', str(a.output / 'test')]
r = subprocess.run(command, capture_output=True, text=True, timeout=30)
(a.output / 'build.log').write_text(r.stdout + r.stderr)
r.check_returncode()
r = subprocess.run([str(a.output / 'test')], capture_output=True, text=True, timeout=30)
(a.output / 'test.log').write_text(r.stdout + r.stderr)
(a.output / 'result.json').write_text(json.dumps(dict(exit=r.returncode,
    hashes={str(f): hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs},
    scope='Actual observation admission/retirement functions; mocked PCI and kernel mutex, no hardware.'), indent=2) + '\n')
print(r.stdout, end='')
print(r.stderr, end='')
r.check_returncode()
