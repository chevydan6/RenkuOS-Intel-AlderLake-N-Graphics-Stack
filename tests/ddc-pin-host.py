#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise the actual DDC pin selector with mocked firmware and device data."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: ddc-pin-host.py path/to/Ports.cpp')
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('addr_t\nPort::_DDCPin()')
end = source.index('\n\nstatus_t\n', start)
prefix = r'''
#include <cstdint>
#include <cstdio>
using uint32 = uint32_t;
using addr_t = uintptr_t;
enum { INTEL_MODEL_ALDM = 1, INTEL_GROUP_BDW = 2, INTEL_PCH_ICP = 3,
 INTEL_PCH_CNP = 2, INTEL_I2C_IO_A = 0x5010, INTEL_I2C_IO_B = 0x5014,
 INTEL_I2C_IO_C = 0x5018, INTEL_I2C_IO_D = 0x501c, INTEL_I2C_IO_E = 0x5020,
 INTEL_I2C_IO_F = 0x5024, INTEL_I2C_IO_I = 0x5030, INTEL_I2C_IO_J = 0x5034,
 INTEL_I2C_IO_K = 0x5038, INTEL_I2C_IO_L = 0x503c,
 INTEL_I2C_IO_M = 0x5040, INTEL_I2C_IO_N = 0x5044 };
struct child_device_config { unsigned ddc_pin; };
struct DeviceType { bool mobile; bool IsModel(int) const { return mobile; }
 bool InGroup(int) const { return false; } int Generation() const { return 12; } };
struct Shared { child_device_config device_configs[1]; DeviceType device_type;
 int pch_info; } shared;
struct Info { Shared* shared_info; } info = { &shared }, *gInfo = &info;
class Port { public: bool present = true; bool _IsPortInVBT(uint32* i)
 { *i = 0; return present; } addr_t _DDCPin(); };
'''
suffix = r'''
int main() {
 Port p; shared.pch_info = INTEL_PCH_ICP; shared.device_type.mobile = true;
 const addr_t expected[] = {0, 0x5014, 0x5018, 0x5034, 0x5038, 0x503c, 0x5040, 0, 0, 0, 0};
 unsigned failures = 0;
 for (unsigned pin = 0; pin < sizeof(expected)/sizeof(expected[0]); ++pin) {
  shared.device_configs[0].ddc_pin = pin;
  addr_t got = p._DDCPin();
  if (got != expected[pin]) { std::fprintf(stderr, "ALDM pin %u: got %lx expected %lx\n", pin, (unsigned long)got, (unsigned long)expected[pin]); ++failures; }
 }
 shared.device_type.mobile = false; shared.device_configs[0].ddc_pin = 4;
 if (p._DDCPin() != 0x5030) { std::fputs("other ICP mapping changed\n", stderr); ++failures; }
 p.present = false;
 if (p._DDCPin() != 0) { std::fputs("missing VBT port accepted\n", stderr); ++failures; }
 if (failures) return 1;
 std::puts("13 DDC pin selection checks passed");
}
'''
with tempfile.TemporaryDirectory(prefix='ddc-pin-test-') as tmp:
    path = pathlib.Path(tmp) / 'test.cpp'
    binary = pathlib.Path(tmp) / 'test'
    path.write_text(prefix + source[start:end] + suffix)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(path), '-o', str(binary)], check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
