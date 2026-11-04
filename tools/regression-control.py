#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Require the unpatched controls to expose exactly the original regressions."""
import subprocess
import sys

checks = [
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
print('PASS all controls reproduce the expected regressions')
