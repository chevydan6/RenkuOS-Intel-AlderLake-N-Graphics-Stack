#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify page-allocation failure cleanup using actual GART method bodies."""
import argparse
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
bundle = parser.parse_args().bundle.resolve()
out = bundle / 'host-gart-allocation'
out.mkdir(exist_ok=False)
for name in ('current', 'original'):
    binary = out / name
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fno-exceptions', '-fno-rtti', '-fsanitize=address,undefined',
                    f'-DGART_ALLOCATION_METHODS="generated/gart-allocation-{name}.inc"',
                    str(bundle / 'tests/gart-allocation-test.cpp'), '-o', str(binary)],
                   check=True)
    result = subprocess.run([str(binary)], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (out / f'{name}.log').write_text(result.stdout)
    expected = ['67 GART allocation checks, 0 failures; mock VM allocation']
    status = 0
    if name == 'original':
        expected = ['FAIL failure clears page pointer',
                    'FAIL failure releases pointer table',
                    'FAIL cleanup releases pointer table',
                    '66 GART allocation checks, 3 failures; mock VM allocation']
        status = 1
    if result.returncode != status or result.stdout.splitlines() != expected:
        raise SystemExit(f'{name} failed validation:\n{result.stdout}')
    print(f'PASS GART allocation {name}: expected exit {status}')
