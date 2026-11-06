#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the real Type-C lifecycle with mocked resource callbacks."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: type-c-power-host.py path/to/accelerant/source')
with tempfile.TemporaryDirectory(prefix='type-c-power-') as tmp:
    stub = pathlib.Path(tmp) / 'SupportDefs.h'
    stub.write_text('#include <stddef.h>\ntypedef int status_t;\n'
                    'enum { B_OK=0, B_ERROR=-1, B_BUSY=-2, B_BAD_VALUE=-3 };\n')
    binary = pathlib.Path(tmp) / 'test'
    test = pathlib.Path(__file__).with_name('type-c-power-test.cpp')
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined',
                    '-I', tmp, '-I', sys.argv[1], str(test), '-o', str(binary)],
                   check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
