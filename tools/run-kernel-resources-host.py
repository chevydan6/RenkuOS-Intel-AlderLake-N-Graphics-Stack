#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Sanitizer test of kernel resource initialization and original-code control."""
import argparse
from collections import Counter
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
bundle = parser.parse_args().bundle.resolve()
out = bundle / 'host-kernel-resources'
out.mkdir(exist_ok=False)
for mode, name in ((mode, name) for mode in ('sanitized', 'optimized')
                   for name in ('current', 'original')):
    binary = out / f'{mode}-{name}'
    flags = ['-fsanitize=address,undefined'] if mode == 'sanitized' else ['-O2', '-fPIC']
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fno-exceptions', '-fno-rtti', *flags,
                    f'-DKERNEL_RESOURCE_CODE="generated/kernel-resources-{name}.inc"',
                    str(bundle / 'tests/kernel-resources-test.cpp'), '-o', str(binary)],
                   check=True)
    result = subprocess.run([str(binary)], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    (out / f'{mode}-{name}.log').write_text(result.stdout)
    expected = Counter() if name == 'current' else Counter({
        'FAIL Alder Lake requests only cursor storage': 20,
        'FAIL Alder Lake ring remains empty': 16,
        'FAIL Alder Lake overlay remains absent': 16,
        'FAIL Alder Lake cursor remains independent': 4,
    })
    failures = Counter(line for line in result.stdout.splitlines() if line.startswith('FAIL '))
    rest = [line for line in result.stdout.splitlines() if not line.startswith('FAIL ')]
    summary = f'280 kernel resource checks, {sum(expected.values())} failures; mock allocation only'
    if result.returncode != bool(expected) or failures != expected or rest != [summary]:
        raise SystemExit(f'{mode} {name} failed validation:\n{result.stdout}')
    print(f'PASS kernel resources {mode} {name}: 280 checks, {sum(expected.values())} expected failures')
