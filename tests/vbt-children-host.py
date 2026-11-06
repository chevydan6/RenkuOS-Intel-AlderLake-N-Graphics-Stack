#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Test the real VBT child-copy routine with malformed and oversized inputs."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: vbt-children-host.py path/to/bios.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('static void\ncopy_vbt_child_configs(')
end = source.index('\n\nstatic void\n', start + 1)
prefix = r'''
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <vector>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t;
#define B_COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
#define min_c(a,b) ((a)<(b)?(a):(b))
#define TRACE(x) ((void)0)
#define DEVICE_NAME "intel_extreme"
struct __attribute__((packed)) child_device_config { uint16 handle, device_type; uint8 rest[12]; };
struct intel_shared_info { uint32 device_config_count; child_device_config device_configs[10]; uint32 guard; };
'''
suffix = r'''
int main() {
 unsigned failures = 0;
 for (unsigned size = 0; size <= 64; ++size) {
  for (unsigned n = 0; n <= 15; ++n) {
   size_t length = size * n + (size ? size - 1 : 0);
   std::vector<uint8> data(length, 0xa5);
   if (size >= 4) for (unsigned i = 0; i < n; ++i) { data[i*size+2]=1; data[i*size+3]=0; }
   intel_shared_info info = {}; info.guard = 0xdeadbeef;
   copy_vbt_child_configs(&info, data.data(), length, size);
   unsigned expected = size < 4 ? 0 : (n < 10 ? n : 10);
   if (info.device_config_count != expected || info.guard != 0xdeadbeef) ++failures;
   if (expected && size < sizeof(child_device_config)) {
    const uint8* bytes = (const uint8*)&info.device_configs[0];
    for (unsigned j = size; j < sizeof(child_device_config); ++j) if (bytes[j]) ++failures;
   }
  }
 }
 uint8 entries[16] = {}; entries[2]=1; entries[10]=0; entries[11]=0;
 intel_shared_info info = {}; info.guard = 0xdeadbeef;
 copy_vbt_child_configs(&info, entries, sizeof(entries), 8);
 if (info.device_config_count != 1 || info.guard != 0xdeadbeef) ++failures;
 if (failures) { std::fprintf(stderr, "%u VBT child failures\n", failures); return 1; }
 std::puts("1,041 VBT child size, capacity, skip and truncation cases passed");
}
'''
with tempfile.TemporaryDirectory(prefix='vbt-child-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
