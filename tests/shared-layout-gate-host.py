#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the accelerant's actual shared-layout compatibility gate."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: shared-layout-gate-host.py path/to/accelerant.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('static bool\nshared_layout_compatible(')
end = source.index('\n\n/*! This is the common accelerant_info', start)
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32=uint32_t;
enum { INTEL_PRIVATE_DATA_MAGIC=0x12345678, INTEL_SHARED_INFO_LAYOUT_VERSION=2 };
struct intel_get_shared_info_layout { uint32 magic, version, shared_info_size; };
struct intel_shared_info { char bytes[1234]; };
'''
suffix = r'''
int main() {
 unsigned errors=0; intel_get_shared_info_layout l={INTEL_PRIVATE_DATA_MAGIC,2,1234};
 if (!shared_layout_compatible(0,l)) errors++;
 if (shared_layout_compatible(-1,l)) errors++;
 l.magic=0; if (shared_layout_compatible(0,l)) errors++; l.magic=INTEL_PRIVATE_DATA_MAGIC;
 l.version=1; if (shared_layout_compatible(0,l)) errors++; l.version=2;
 l.shared_info_size=1233; if (shared_layout_compatible(0,l)) errors++;
 l.shared_info_size=2000; if (!shared_layout_compatible(0,l)) errors++;
 if(errors){std::fprintf(stderr,"%u layout-gate cases failed\n",errors);return 1;}
 std::puts("6 shared-layout compatibility gate cases passed");
}
'''
with tempfile.TemporaryDirectory(prefix='shared-gate-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
