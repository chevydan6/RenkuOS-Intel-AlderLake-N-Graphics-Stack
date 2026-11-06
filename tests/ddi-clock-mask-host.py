#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Test real clock-selection register writes with mocked register access."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: ddi-clock-mask-host.py path/to/Ports.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('uint32 config = read32(TGL_DPCLKA_CFGCR0);')
end = source.index('\n\t\tTRACE("PLL configuration after changes:', start)
snippet = source[start:end]
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32 = uint32_t;
#define TRACE(...) ((void)0)
enum { TGL_DPCLKA_CFGCR0 = 0, TGL_DPCLKA_DDIA_CLOCK_OFF = 1 << 10,
 TGL_DPCLKA_DDIB_CLOCK_OFF = 1 << 11,
 TGL_DPCLKA_DDIA_CLOCK_SELECT = 3 << 0,
 TGL_DPCLKA_DDIB_CLOCK_SELECT = 3 << 2,
 TGL_DPCLKA_DDIB_CLOCK_SELECT_SHIFT = 2 };
uint32 initial, writes[2]; unsigned count;
uint32 read32(uint32) { return initial; }
void write32(uint32, uint32 value) { if (count < 2) writes[count] = value; ++count; }
void configure(int chosenPLL) {
'''
suffix = r'''
}
int main() {
 unsigned failures = 0;
 for (unsigned pll = 0; pll < 2; ++pll) {
  uint32 off = pll ? TGL_DPCLKA_DDIB_CLOCK_OFF : TGL_DPCLKA_DDIA_CLOCK_OFF;
  uint32 mask = pll ? TGL_DPCLKA_DDIB_CLOCK_SELECT : TGL_DPCLKA_DDIA_CLOCK_SELECT;
  uint32 selection = pll ? 1u << TGL_DPCLKA_DDIB_CLOCK_SELECT_SHIFT : 0;
  for (uint32 value = 0; value < 65536; ++value) {
   initial = value | 0xA5A50000u; count = 0; configure(pll);
   uint32 gated = ((initial | off) & ~mask) | selection;
   if (count != 2 || writes[0] != gated || writes[1] != (gated & ~off)) {
    if (failures < 4) std::fprintf(stderr, "PLL %u input %08x: wrong register writes\n", pll, initial);
    ++failures;
   }
  }
 }
 if (failures) { std::fprintf(stderr, "%u failures\n", failures); return 1; }
 std::puts("131072 clock mask and gating checks passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ddi-clock-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + snippet + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
