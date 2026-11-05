#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the real EDID/pipe-selection branch with mock pipe registers."""
import pathlib, subprocess, sys, tempfile
if len(sys.argv) != 2:
    raise SystemExit('usage: ddi-pipe-host.py path/to/Ports.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('bool\nDigitalDisplayInterface::IsConnected()')
branch = source.rindex('\n\tif (edidDetected) {', start, source.index('\nstatus_t\n', start))
brace = source.index('{', branch)
depth = 1
end = brace + 1
while depth:
    if source[end] == '{': depth += 1
    elif source[end] == '}': depth -= 1
    end += 1
snippet = source[branch:end]
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32=uint32_t;
#define B_PRIx32 "x"
#define TRACE(...) ((void)0)
enum { PIPE_DDI_FUNC_CTL_A=0, PIPE_DDI_FUNC_CTL_B=1, PIPE_DDI_FUNC_CTL_C=2 };
constexpr uint32 PIPE_DDI_FUNC_CTL_ENABLE=0x80000000u;
uint32 state[3];
uint32 read32(uint32 reg) { return state[reg]; }
bool branch(bool edidDetected) {
 uint32 pipeState=0;
'''
suffix = r'''
 return false;
}
int main() {
 unsigned failures=0;
 for (unsigned mask=0;mask<8;mask++) {
  for (unsigned i=0;i<3;i++) state[i]=(mask&(1u<<i))?PIPE_DDI_FUNC_CTL_ENABLE:0;
  bool got=branch(true), expected=mask!=7;
  if(got!=expected){std::fprintf(stderr,"mask %u: got %d, expected %d\n",mask,got,expected);failures++;}
  if(branch(false)){std::fprintf(stderr,"missing EDID accepted for mask %u\n",mask);failures++;}
 }
 if(failures) return 1;
 std::puts("16 EDID/pipe availability checks passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ddi-pipe-test-') as tmp:
    path=pathlib.Path(tmp)/'test.cpp'; binary=pathlib.Path(tmp)/'test'
    path.write_text(prefix+snippet+suffix)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(path),'-o',str(binary)],check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
