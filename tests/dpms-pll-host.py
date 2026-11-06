#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise the actual DPMS function with simulated legacy PLL registers."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: dpms-pll-host.py path/to/dpms.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('void\nset_display_power_mode(uint32 mode)')
end = source.index('\n\n//\t#pragma mark -', start)
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32=uint32_t;
enum { INTEL_GROUP_ALD=1, B_DPMS_ON=0, B_DPMS_SUSPEND=1, B_DPMS_STAND_BY=2, B_DPMS_OFF=3,
 INTEL_DISPLAY_A_PLL=1, INTEL_DISPLAY_B_PLL=2, INTEL_ANALOG_PORT=3, INTEL_DIGITAL_PORT_B=4,
 INTEL_DISPLAY_A_BASE=5, DISPLAY_PLL_ENABLED=0x100, DISPLAY_MONITOR_ON=0,
 DISPLAY_MONITOR_SUSPEND=1, DISPLAY_MONITOR_STAND_BY=2, DISPLAY_MONITOR_OFF=3,
 DISPLAY_MONITOR_MODE_MASK=3, DISPLAY_MONITOR_PORT_ENABLED=4, HEAD_MODE_A_ANALOG=1,
 HEAD_MODE_LVDS_PANEL=2 };
struct Device { bool ald; bool InGroup(int) {return ald;} };
struct Shared { Device device_type; } shared; struct Info { Shared* shared_info; int head_mode; } info={&shared,0}, *gInfo=&info;
uint32 regs[6]; unsigned pllWrites, pipeCalls; bool lastPipe;
uint32 read32(uint32 reg) {return regs[reg];}
void write32(uint32 reg,uint32 value) { if(reg==1||reg==2) pllWrites++; regs[reg]=value; }
void spin(int) {} void wait_for_vblank() {} void enable_lvds_panel(bool) {}
void enable_all_pipes(bool value) {pipeCalls++;lastPipe=value;}
'''
suffix = r'''
int main() {
 unsigned failures=0;
 for (unsigned ald=0;ald<2;ald++) for (unsigned mode=0;mode<=3;mode++) {
  for (auto& reg:regs) reg=0;
  pllWrites=pipeCalls=0; lastPipe=false; shared.device_type.ald=ald;
  set_display_power_mode(mode);
  unsigned expected=ald?0:(mode==B_DPMS_ON?6:mode==B_DPMS_OFF?2:0);
  if(pllWrites!=expected || pipeCalls!=1 || lastPipe!=(mode==B_DPMS_ON)) failures++;
 }
 if(failures){std::fprintf(stderr,"%u DPMS cases failed\n",failures);return 1;}
 std::puts("8 Alder Lake and legacy DPMS PLL cases passed");
}
'''
with tempfile.TemporaryDirectory(prefix='dpms-pll-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
