#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Validate native RCS compiler artifact schema and supported initial profile."""
import json
import hashlib
from pathlib import Path
import sys

ARCHIVE = '91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a'

def require(condition):
    if not condition:
        raise ValueError("artifact violates supported N100 shader schema")

def check(root):
    m = json.loads((root / 'metadata.json').read_text())
    require(m['schema_version'] == 1)
    require(m['mesa_version'] == '26.3.0' and m['mesa_archive_sha256'] == ARCHIVE)
    require(m['pci_device_id'] == 0x46d1 and m['graphics_verx10'] == 120)
    require(m['harness_sha256'] == hashlib.sha256(Path(__file__).with_name('native_rcs_shaders.c').read_bytes()).hexdigest())
    require(m['constant_color'] == [1, .25, 0, 1])
    for name in ('vs', 'fs'):
        p = m[name]
        code = (root / (name + '.bin')).read_bytes()
        require(len(code) == p['program_size'] and len(code) >= 16)
        require(len(code) % 8 == 0 and any(code))
        require(p['total_scratch'] == p['num_relocs'] == 0)
        require(p['total_shared'] == 0 and p['const_data_size'] == 0)
        require(p['grf_used'] > 0 and p['dispatch_grf_start_reg'] >= 0)
        require(p['push_sizes'] == [0, 0, 0, 0])
        nir = (root / (name + '.nir')).read_text()
        require('shader: MESA_SHADER_' in nir and 'impl main' in nir)
    v, f = m['vs'], m['fs']
    require(int(v['inputs_read'], 16) == 1 << m['position_attribute'])
    require(v['dispatch_mode'] == 3)
    require(not v['include_vue_handles'] and not v['clip_distance_mask'] and not v['cull_distance_mask'])
    require(v['urb_entry_size'] > 0 and v['urb_read_length'] >= 0)
    # Mesa 26.3.0 brw_compute_vue_map(): this shader writes only position.
    # Slot 0 is the PSIZ header (12); POS (0) is slot 1. Header aliases
    # PRIMITIVE_SHADING_RATE (22), LAYER (23), VIEWPORT (24) share slot 0.
    expected_vue = [-1] * 64
    for varying, slot in {0: 1, 12: 0, 22: 0, 23: 0, 24: 0}.items():
        expected_vue[varying] = slot
    require(int(v['vue_slots_valid'], 16) == 1)
    require(v['vue_num_slots'] == 2 and v['slot_to_varying'] == [12, 0])
    require(v['varying_to_slot'] == expected_vue)
    require(len(f['urb_setup']) == 64 and all(slot == -1 for slot in f['urb_setup']))
    require(int(f['inputs'], 16) == f['num_varying_inputs'] == f['barycentric_interp_modes'] == 0)
    require(all(f[field] == 0 for field in ('computed_depth_mode', 'computed_stencil', 'uses_kill', 'uses_omask', 'uses_src_depth', 'uses_src_w', 'persample_dispatch', 'coarse_pixel_dispatch', 'has_side_effects')))
    require(f['dispatch_8'] or f['dispatch_16'] or f['dispatch_32'])
    enabled_offsets = [0] if f['dispatch_8'] else []
    for width in (16, 32):
        if f[f'dispatch_{width}']:
            require(0 <= f[f'prog_offset_{width}'] < f['program_size'])
            require(f[f'prog_offset_{width}'] % 64 == 0)
            enabled_offsets.append(f[f'prog_offset_{width}'])
    require(len(enabled_offsets) == len(set(enabled_offsets)))
    print('PASS: N100 compiled VS/FS artifacts, zero scratch and relocations')

if __name__ == '__main__':
    try:
        check(Path(sys.argv[1]))
    except (AssertionError, KeyError, ValueError, OSError, IndexError) as exc:
        print(f'FAIL: {type(exc).__name__}: {exc}', file=sys.stderr)
        sys.exit(1)
