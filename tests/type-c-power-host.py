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
    stub.write_text('#pragma once\n#include <stddef.h>\n#include <stdint.h>\n'
                    'typedef int status_t; typedef uint8_t uint8; typedef uint32_t uint32; typedef uint64_t uint64;\n'
                    'enum { B_OK=0, B_ERROR=-1, B_BUSY=-2, B_BAD_VALUE=-3, B_TIMED_OUT=-4 };\n')
    for name in ('type-c-power-test', 'type-c-well-test', 'type-c-transcoder-test',
                 'alder-lake-tc-backend-test', 'dekel-hdmi-pll-test',
                 'dekel-pll-programmer-test', 'type-c-transcoder-programmer-test',
                 'type-c-buffer-test', 'dekel-hdmi-signal-test',
                 'dekel-signal-programmer-test', 'alder-lake-hdmi-sequence-test'):
        binary = pathlib.Path(tmp) / name
        test = pathlib.Path(__file__).with_name(name + '.cpp')
        subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined',
                        '-I', tmp, '-I', sys.argv[1], str(test), '-o', str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
