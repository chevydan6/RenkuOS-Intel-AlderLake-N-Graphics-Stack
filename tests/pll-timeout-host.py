#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run the actual PLL routine against simulated power and lock failures."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: pll-timeout-host.py path/to/TigerLakePLL.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('static bool\nwait_for_pll_state(')
end = source.index('\n\n/**', start)
helper = source[start:end]
start = source.index('status_t\nProgramPLL(')
function = source[start:]
prefix = r'''
#include <cmath>
#include <cstdint>
#include <cstdio>
using uint32 = uint32_t; using int32 = int32_t; using status_t = int;
enum { B_OK = 0, B_BAD_VALUE = -1, B_TIMED_OUT = -2 };
enum { TGL_DPLL0_ENABLE=0, TGL_DPLL0_SPREAD_SPECTRUM=1, TGL_DPLL0_CFGCR0=2, TGL_DPLL0_CFGCR1=3,
 TGL_DPLL1_ENABLE=4, TGL_DPLL1_SPREAD_SPECTRUM=5, TGL_DPLL1_CFGCR0=6, TGL_DPLL1_CFGCR1=7,
 TGL_DPLL4_ENABLE=8, TGL_DPLL4_SPREAD_SPECTRUM=9, TGL_DPLL4_CFGCR0=10, TGL_DPLL4_CFGCR1=11,
 TGL_DPLL_LOCK=1, TGL_DPLL_ENABLE=2, TGL_DPLL_POWER_ENABLE=4, TGL_DPLL_POWER_STATE=8,
 TGL_DPLL_SSC_ENABLE=16, TGL_DPLL_DCO_FRACTION_SHIFT=8,
 TGL_DPLL_PDIV_2=1, TGL_DPLL_PDIV_3=2, TGL_DPLL_PDIV_5=3, TGL_DPLL_PDIV_7=4,
 TGL_DPLL_KDIV_1=16, TGL_DPLL_KDIV_2=32, TGL_DPLL_KDIV_3=48,
 TGL_DPLL_QDIV_RATIO_SHIFT=8, TGL_DPLL_QDIV_ENABLE=64 };
#define B_PRIx32 "x"
#define B_PRId32 "d"
#define TRACE(...) ((void)0)
#define ERROR(...) ((void)0)
struct PLL { int reference_frequency; }; struct Shared { PLL pll_info; } shared = {{19200}};
struct Info { Shared* shared_info; } info = {&shared}, *gInfo = &info;
uint32 registers[12]; int phase, failure, sleeps;
uint32 read32(uint32 reg) {
 if (reg == 0) {
  uint32 value = registers[0] & ~(TGL_DPLL_LOCK | TGL_DPLL_POWER_STATE);
  if (phase == 1 && failure == 1) value |= TGL_DPLL_LOCK;
  if (phase >= 2 && failure != 2) value |= TGL_DPLL_POWER_STATE;
  if (phase == 3 && failure != 3) value |= TGL_DPLL_LOCK;
  return value;
 }
 return registers[reg];
}
void write32(uint32 reg, uint32 value) {
 registers[reg] = value;
 if (reg == 0) phase = (value & TGL_DPLL_ENABLE) ? 3 :
  (value & TGL_DPLL_POWER_ENABLE) ? 2 : 1;
}
void snooze(int) { sleeps++; }
'''
suffix = r'''
int main() {
 for (int fail = 0; fail <= 3; fail++) {
  for (auto& r : registers) r = 0;
  phase = 0; failure = fail; sleeps = 0;
  status_t got = ProgramPLL(0, 2, 1, 2, 8910.0f);
  if (got != (fail ? B_TIMED_OUT : B_OK) || (fail && sleeps != 2000)
      || phase != (fail ? fail : 3)) {
   std::fprintf(stderr, "failure stage %d: status %d phase %d sleeps %d\n", fail, got, phase, sleeps);
   return 1;
  }
 }
 if (ProgramPLL(9, 2, 1, 2, 8910.0f) != B_BAD_VALUE) return 1;
 std::puts("5 PLL success, timeout and invalid-PLL checks passed");
}
'''
with tempfile.TemporaryDirectory(prefix='pll-timeout-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + helper + '\n' + function + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
