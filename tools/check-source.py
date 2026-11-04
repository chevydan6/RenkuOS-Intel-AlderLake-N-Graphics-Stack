#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Offline checks after prepare.py: coverage, control failure, payload hashes."""
import hashlib
import json
import pathlib
import subprocess
import sys

bundle = pathlib.Path(sys.argv[1]).resolve()
manifest = json.loads((bundle / 'manifest.json').read_text())
for relative, expected in manifest['payload_sha256'].items():
    actual = hashlib.sha256((bundle / relative).read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f'payload mismatch: {relative}')
subprocess.run([sys.executable, str(bundle / 'tests/source-audit.py'), str(bundle / 'source')], check=True)
result = subprocess.run([sys.executable, str(bundle / 'tests/source-audit.py'), str(bundle / 'controls/intel_gart.cpp')], capture_output=True, text=True)
failures = [line for line in result.stdout.splitlines() if line.startswith('FAIL ')]
if result.returncode != 1 or failures != ['FAIL verified N100 pair', 'FAIL N100 pair unique']:
    raise SystemExit('unpatched GART control did not reproduce expected failures:\n' + result.stdout + result.stderr)
print(f"PASS {len(manifest['payload_sha256'])} payload hashes and unpatched GART regression")
print('Native compilation, linked hook tests and hardware execution are separate checks.')
