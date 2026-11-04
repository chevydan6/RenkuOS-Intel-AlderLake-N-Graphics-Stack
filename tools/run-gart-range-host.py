#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Sanitizer checks of extracted GART methods, including the original-code control."""
import argparse
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('bundle', type=pathlib.Path)
args = parser.parse_args()
bundle = args.bundle.resolve()
out = bundle / 'host-gart-range'
out.mkdir(exist_ok=False)
source = bundle / 'tests/gart-range-test.cpp'
flags = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fno-exceptions',
         '-fno-rtti', '-fsanitize=address,undefined']
for name in ('current', 'original'):
    binary = out / name
    subprocess.run(flags + [f'-DGART_METHODS="generated/gart-{name}.inc"',
                            str(source), '-o', str(binary)], check=True)
    run = subprocess.run([str(binary)], stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True)
    (out / f'{name}.log').write_text(run.stdout)
    failures = [line for line in run.stdout.splitlines() if line.startswith('FAIL ')]
    expected = [] if name == 'current' else [
        'FAIL unbind adjusted page count',
        'FAIL unbind preserves reserved and adjacent mappings'] * 3
    expected_status = 0 if name == 'current' else 1
    if run.returncode != expected_status or failures != expected:
        raise SystemExit(f'{name} failed validation:\n{run.stdout}')
    print(f'PASS GART {name}: status {run.returncode}, {len(failures)} expected failures')
