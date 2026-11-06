#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the real DDI register selector against mocked device types."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: ddi-port-register-host.py path/to/Ports.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('addr_t\nDigitalDisplayInterface::_PortRegister()')
end = source.index('\n\naddr_t\n', start)
prefix = r'''
#include <cstdint>
#include <cstdio>
using addr_t = uintptr_t;
enum { INTEL_PORT_A=1, INTEL_PORT_B, INTEL_PORT_C, INTEL_PORT_D,
 INTEL_PORT_E, INTEL_PORT_F, INTEL_PORT_G, INTEL_MODEL_ALDM=30, INTEL_GROUP_SKY=31,
 DDI_BUF_CTL_A=0x64000, DDI_BUF_CTL_B=0x64100, DDI_BUF_CTL_C=0x64200,
 DDI_BUF_CTL_D=0x64300, DDI_BUF_CTL_E=0x64400, DDI_BUF_CTL_F=0x64500,
 DDI_BUF_CTL_G=0x64600 };
struct Type { bool aldm; int generation; bool IsModel(int) const {return aldm;}
 int Generation() const {return generation;} bool InGroup(int) const {return false;} };
struct Shared { Type device_type; } shared; struct Info { Shared* shared_info; } info = {&shared}, *gInfo = &info;
class DigitalDisplayInterface { public: int port; int PortIndex() {return port;} addr_t _PortRegister(); };
'''
suffix = r'''
int main() {
 DigitalDisplayInterface p; unsigned failures = 0; shared.device_type.generation = 12;
 for (int model = 0; model <= 1; ++model) {
  shared.device_type.aldm = model;
  for (int port = INTEL_PORT_A; port <= INTEL_PORT_G; ++port) {
   p.port = port; addr_t expected = 0x64000 + (port - 1) * 0x100;
   if (model && port == INTEL_PORT_F) expected = DDI_BUF_CTL_D;
   if (model && port == INTEL_PORT_G) expected = DDI_BUF_CTL_E;
   if (p._PortRegister() != expected) ++failures;
  }
 }
 shared.device_type.aldm = false; shared.device_type.generation = 8; p.port = INTEL_PORT_G;
 if (p._PortRegister() != 0) ++failures;
 if (failures) { std::fprintf(stderr, "%u DDI register cases failed\n", failures); return 1; }
 std::puts("15 DDI register selection cases passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ddi-register-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
