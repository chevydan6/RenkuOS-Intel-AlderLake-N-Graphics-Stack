#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Test the source driver's display-port selector without touching hardware."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: ddi-port-select-host.py path/to/Ports.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('static bool\nddi_pipe_selects_port(')
end = source.index('\n\nstatic bool\n', start + 1)
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32 = uint32_t;
enum port_index { INTEL_PORT_ANY, INTEL_PORT_A, INTEL_PORT_B, INTEL_PORT_C,
 INTEL_PORT_D, INTEL_PORT_E, INTEL_PORT_F, INTEL_PORT_G };
#define PIPE_DDI_SELECT_SHIFT 28
#define PIPE_DDI_SELECT_MASK (7u << 28)
#define TGL_PIPE_DDI_SELECT_SHIFT 27
#define TGL_PIPE_DDI_SELECT_MASK (15u << 27)
'''
suffix = r'''
int main() {
 unsigned failures = 0;
 for (unsigned value = 0; value < 16; ++value) {
  for (unsigned port = INTEL_PORT_A; port <= INTEL_PORT_G; ++port) {
   for (unsigned enabled = 0; enabled < 2; ++enabled) {
    uint32 state = (value << 27) | (enabled << 31) | 0x12345u;
    bool got = ddi_pipe_selects_port(state, 12, (port_index)port, false);
    if (got != (value == port)) ++failures;
   }
  }
 }
 for (unsigned value = 0; value < 8; ++value)
  for (unsigned port = INTEL_PORT_A; port <= INTEL_PORT_G; ++port)
   if (ddi_pipe_selects_port((value << 28) | 0x12345u, 9, (port_index)port, false)
       != (value + 1 == port)) ++failures;
 for (unsigned value = 0; value < 16; ++value) {
  uint32 state = value << 27;
  if (ddi_pipe_selects_port(state, 12, INTEL_PORT_F, true) != (value == INTEL_PORT_D)) ++failures;
  if (ddi_pipe_selects_port(state, 12, INTEL_PORT_G, true) != (value == INTEL_PORT_E)) ++failures;
 }
 if (failures) { std::fprintf(stderr, "%u selector failures\n", failures); return 1; }
 std::puts("312 Gen12, legacy and Alder Lake mobile port-selection checks passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ddi-port-select-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
