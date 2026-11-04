#!/usr/bin/env python3
"""Reject unexpected kernel dependencies and wrong ELF/symbol outputs."""
import hashlib
import pathlib
import re
import subprocess
import sys

out = pathlib.Path(sys.argv[1])
for name, symbol in [('intel_gart', 'modules'), ('intel_extreme.accelerant', 'get_accelerant_hook')]:
    path = out / name
    header = subprocess.check_output(['readelf', '-h', str(path)], text=True)
    if 'ELF64' not in header or 'Advanced Micro Devices X86-64' not in header or 'DYN' not in header:
        raise SystemExit(f'{name}: unexpected ELF class, architecture or type')
    symbols = subprocess.check_output(['nm', '-D', '--defined-only', str(path)], text=True)
    if not re.search(r'\b' + symbol + r'$', symbols, re.M):
        raise SystemExit(f'{name}: missing entry point {symbol}')
    dynamic = subprocess.check_output(['readelf', '-d', str(path)], text=True)
    needed = re.findall(r'\(NEEDED\).*\[(.*?)\]', dynamic)
    if name == 'intel_gart' and needed != ['_KERNEL_']:
        raise SystemExit(f'unexpected kernel dependencies: {needed}')
    print(f'PASS {name}: {path.stat().st_size} bytes, sha256={hashlib.sha256(path.read_bytes()).hexdigest()}')
    print(f'  dynamic dependencies: {needed}')
