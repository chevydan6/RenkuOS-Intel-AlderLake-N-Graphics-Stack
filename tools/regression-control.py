#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Require the unpatched controls to expose exactly the original regressions."""
import subprocess
import sys
from collections import Counter

checks = [
    (['build/gart-allocation-control'],
     ['FAIL failure clears page pointer',
      'FAIL failure releases pointer table', 'FAIL cleanup releases pointer table']),
    (['build/gart-range-control'],
     ['FAIL unbind adjusted page count',
      'FAIL unbind preserves reserved and adjacent mappings'] * 3),
    (['build/gating-control'], ['FAIL Alder Lake gating unsupported',
                              'FAIL Alder Lake gating registers preserved']),
    ([sys.executable, 'tests/source-audit.py', 'controls/intel_gart.cpp'],
     ['FAIL verified N100 pair', 'FAIL N100 pair unique']),
    (['build/hooks-test', './build/intel_extreme.control.accelerant'],
     ['FAIL drawing capability'] * 21),
    (['build/lifecycle-test', './build/intel_extreme.lifecycle-control.accelerant'],
     ['FAIL Alder Lake engine count', 'FAIL Alder Lake ring state',
      'FAIL Alder Lake register writes']),
]
for command, expected in checks:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    failures = [line for line in result.stdout.splitlines() if line.startswith('FAIL ')]
    if (result.returncode != 1 or len(failures) != len(expected)
            or any(not actual.startswith(wanted) for actual, wanted in zip(failures, expected))):
        print(result.stdout)
        raise SystemExit('control did not reproduce exactly the expected regression')
    print(f'PASS unpatched control: {len(failures)} expected failures: {command[0]}')

run = subprocess.run(['build/gart-lifecycle-control'], stdout=subprocess.PIPE,
                     stderr=subprocess.STDOUT, text=True)
expected = Counter({
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
actual = Counter(line for line in run.stdout.splitlines() if line.startswith('FAIL '))
if run.returncode != 1 or actual != expected:
    raise SystemExit('GART lifecycle control failed:\n' + run.stdout)
print('PASS GART lifecycle original control: 39 expected failures')
print('PASS all controls reproduce the expected regressions')
