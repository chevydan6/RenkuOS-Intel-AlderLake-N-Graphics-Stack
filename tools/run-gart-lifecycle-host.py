#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fault-inject production GART lifecycle algorithms with sanitizer coverage."""
import argparse
from collections import Counter
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
args = parser.parse_args()
bundle = args.bundle.resolve()
out = bundle / 'host-gart-lifecycle'
out.mkdir(exist_ok=False)
source = bundle / 'tests/gart-lifecycle-test.cpp'
flags = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fno-exceptions',
         '-fno-rtti', '-fsanitize=address,undefined']
expected_original = Counter({
    'FAIL no free with owned PTEs installed': 14,
    'FAIL completed cleanup removes every owned PTE': 8,
    'FAIL cleanup preserves owned backing on failure': 6,
    'FAIL cleanup preserves allocation metadata': 6,
    'FAIL failed borrowed bind clears installed prefix': 1,
    'FAIL failed borrowed bind removes new metadata after cleanup': 1,
    'FAIL public reserved rebind rejected': 1,
    'FAIL rejected public rebind preserves output handle': 1,
    'FAIL rejected public rebind preserves existing mapping': 1,
})
for name in ('current', 'original'):
    binary = out / name
    defines = [f'-DGART_LIFECYCLE_METHODS="generated/gart-lifecycle-{name}.inc"']
    if name == 'original':
        defines += ['-DGART_DELETE_RESULT=void', '-DGART_HAS_DELETE_ALL=0']
    subprocess.run(flags + defines + [str(source), '-o', str(binary)], check=True)
    run = subprocess.run([str(binary)], stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True)
    (out / f'{name}.log').write_text(run.stdout)
    lines = run.stdout.splitlines()
    failures = Counter(line for line in lines if line.startswith('FAIL '))
    remainder = [line for line in lines if not line.startswith('FAIL ')]
    expected = Counter() if name == 'current' else expected_original
    checks, count, status = (154, 0, 0) if name == 'current' else (103, 39, 1)
    summary = f'{checks} GART lifecycle checks, {count} failures; simulated backing and bus faults'
    if run.returncode != status or failures != expected or remainder != [summary]:
        raise SystemExit(f'{name} failed validation:\n{run.stdout}')
    print(f'PASS GART lifecycle {name}: {checks} checks, {count} expected failures')
