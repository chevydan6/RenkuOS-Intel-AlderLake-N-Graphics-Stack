#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Integrate into a fresh Mesa 26.3.0 tree; no enumerated compiler objects."""
from pathlib import Path
import hashlib
import shutil
import sys
src = Path(__file__).resolve().parent
tree = Path(sys.argv[1]).resolve()
assert (tree / 'VERSION').read_text().strip() == '26.3.0'
p = tree / 'meson.build'
s = p.read_text()
needle = 'with_any_intel = ['
if needle in s:
    s = s.replace(needle, 'with_any_intel = true or [', 1)
else:
    assert 'with_any_intel = true or [' in s
compute = 'with_gfx_compute = ['
if compute in s:
    s = s.replace(compute, 'with_gfx_compute = true or [', 1)
else:
    assert 'with_gfx_compute = true or [' in s
p.write_text(s)
brw = tree / 'src/intel/compiler/brw'
p = brw / 'meson.build'
s = p.read_text()
if "executable('native-rcs-shaders'" not in s:
    p.write_text(s + '''
# Offline native RCS shader compiler: use Mesa's declared compiler dependencies.
executable('native-rcs-shaders', 'native_rcs_shaders.c',
 include_directories: [inc_include, inc_src, inc_intel, inc_intel_compiler],
 dependencies: [idep_intel_compiler_brw, idep_intel_dev],
 install: false)
''')
shutil.copyfile(src / 'native_rcs_shaders.c', brw / 'native_rcs_shaders.c')
digest = hashlib.sha256((src / 'native_rcs_shaders.c').read_bytes()).hexdigest()
(brw / 'source_identity.h').write_text(f'#define NATIVE_RCS_SOURCE_SHA256 "{digest}"\n')
