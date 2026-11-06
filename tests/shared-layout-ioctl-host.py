#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise the actual shared-layout ioctl with mocked user copies."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: shared-layout-ioctl-host.py path/to/device.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('case INTEL_GET_SHARED_INFO_LAYOUT:')
end = source.index('\n\t\tcase ', start + 1)
case = source[start:end]
prefix = r'''
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
using uint32=uint32_t; using status_t=int;
enum { B_OK=0, B_BAD_VALUE=-1, B_BAD_ADDRESS=-2, INTEL_GET_SHARED_INFO_LAYOUT=10,
 INTEL_PRIVATE_DATA_MAGIC=0x12345678, INTEL_SHARED_INFO_LAYOUT_VERSION=2 };
struct intel_get_shared_info_layout { uint32 magic, version, shared_info_size; };
struct intel_shared_info { char bytes[1234]; };
int copies, failCopy;
status_t user_memcpy(void* dst, const void* src, size_t size) {
 copies++; if (!dst || !src || copies == failCopy) return B_BAD_ADDRESS;
 std::memcpy(dst, src, size); return B_OK;
}
status_t run(void* buffer, size_t bufferLength) { switch (INTEL_GET_SHARED_INFO_LAYOUT) {
'''
suffix = r'''
default: return B_BAD_VALUE; } }
int main() {
 unsigned errors=0; intel_get_shared_info_layout l={INTEL_PRIVATE_DATA_MAGIC,99,99};
 copies=0; failCopy=0;
 if (run(&l,sizeof(l))!=B_OK || l.version!=2 || l.shared_info_size!=1234 || copies!=2) errors++;
 l={INTEL_PRIVATE_DATA_MAGIC,99,99}; copies=0;
 if (run(&l,sizeof(l)-1)!=B_BAD_VALUE || copies || l.version!=99) errors++;
 l={0,99,99}; copies=0;
 if (run(&l,sizeof(l))!=B_BAD_VALUE || copies!=1 || l.version!=99) errors++;
 copies=0; if (run(nullptr,sizeof(l))!=B_BAD_ADDRESS) errors++;
 l={INTEL_PRIVATE_DATA_MAGIC,99,99}; copies=0; failCopy=2;
 if (run(&l,sizeof(l))!=B_BAD_ADDRESS || l.version!=99) errors++;
 if(errors){std::fprintf(stderr,"%u ioctl cases failed\n",errors);return 1;}
 std::puts("5 shared-layout ioctl validation cases passed");
}
'''
with tempfile.TemporaryDirectory(prefix='shared-layout-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + case + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
