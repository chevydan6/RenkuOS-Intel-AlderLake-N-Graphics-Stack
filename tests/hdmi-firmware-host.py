#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile the real firmware gate against a minimal shared-info fixture."""
import pathlib
import subprocess
import sys
import tempfile

if len(sys.argv) != 2:
    raise SystemExit('usage: hdmi-firmware-host.py path/to/accelerant/source')
with tempfile.TemporaryDirectory(prefix='hdmi-firmware-') as tmp:
    stub = pathlib.Path(tmp) / 'intel_extreme.h'
    stub.write_text(r'''
#pragma once
#include <cstddef>
#include <cstdint>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using status_t=int;
enum { B_OK=0, B_BAD_VALUE=-1, INTEL_MODEL_ALDM=1, DEVICE_TYPE_TMDS_DVI_SIGNALING=16,
 DVO_PORT_HDMIF=14, DVO_PORT_HDMIG=16 };
enum port_index { INTEL_PORT_ANY, INTEL_PORT_A, INTEL_PORT_B, INTEL_PORT_C,
 INTEL_PORT_D, INTEL_PORT_E, INTEL_PORT_F, INTEL_PORT_G };
#define B_COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
struct DeviceType { bool aldm; bool IsModel(int) const {return aldm;} };
struct __attribute__((packed)) child_device_config {
 uint16 handle, device_type; uint8 device_id[10]; uint16 addin_offset;
 uint8 dvo_port, i2c_pin, slave_addr, ddc_pin; uint16 edid_ptr; uint8 dvo_cfg;
 bool efp_routed:1; bool lane_reversal:1; uint8 rest:6;
};
struct pll_info { uint32 reference_frequency; };
struct intel_shared_info { bool got_vbt; DeviceType device_type; pll_info pll_info;
 uint32 device_config_count; child_device_config device_configs[10];
 uint32 type_c_port_leases, vbt_bdb_version, vbt_child_device_size, gpu_pci_vendor, gpu_pci_device, gpu_pci_revision; };
'''.replace('pll_info pll_info;', 'struct pll_info pll_info;'))
    binary = pathlib.Path(tmp) / 'test'
    test = pathlib.Path(__file__).with_name('alder-lake-hdmi-firmware-test.cpp')
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', tmp, '-I', sys.argv[1],
                    str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
