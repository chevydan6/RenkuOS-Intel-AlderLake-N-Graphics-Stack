#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile against real pinned RenkuOS headers, excluding host libc."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--cc', default='gcc')
a = p.parse_args()
root = Path(__file__).resolve().parent
source = a.source.resolve()
revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip()
if revision != '4799bb5a188c4bb6bbef967287640cc21a061a0d':
    p.error('expected the GPU branch RenkuOS source baseline 4799bb5')
subprocess.run(['python3', str(root / 'verify-upstream.py')], check=True)
subprocess.run(['python3', str(root / 'extract.py')], check=True)
subprocess.run(['python3', str(root / 'extract-emitter.py')], check=True)
gcc_include = subprocess.check_output([a.cc, '-print-file-name=include'], text=True).strip()
flags = ['-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
         '-Wno-sign-compare', '-fno-strict-overflow',
         '-ffreestanding', '-fno-builtin', '-fno-stack-protector', '-mno-red-zone',
         '-fno-tree-vectorize', '-mgeneral-regs-only', '-nostdinc',
         '-isystem', gcc_include, '-DRENKU_NATIVE']
for directory in ['headers', 'headers/os', 'headers/os/support', 'headers/os/kernel',
                  'headers/os/storage', 'headers/os/drivers', 'headers/posix']:
    flags += ['-I' + str(source / directory)]
flags += ['-I' + str(root / 'include'), '-I' + str(root / 'upstream')]
build = root / 'build/target'
build.mkdir(exist_ok=True)
records = []
for unit in ['build/intel_ring_slice.c', 'build/gen8_engine_slice.c', 'upstream/drm/drm_mm.c',
             'upstream/lib/rbtree.c', 'upstream/lib/refcount.c',
             'upstream/linux/ww_mutex.h',
             'src/native_locks.c', 'src/task_identity.c', 'src/time.c', 'src/native_diagnostics.c']:
    obj = build / (Path(unit).stem + '.o')
    cmd = [a.cc, *flags, '-MMD', '-c', str(root / unit), '-o', str(obj)]
    if unit.endswith('.h'):
        # Emit the unchanged header's real inline code, not a precompiled header.
        cmd[1:1] = ['-x', 'c', '-fkeep-inline-functions']
    print(' '.join(cmd), flush=True)
    subprocess.run(cmd, check=True)
    records.append({'source': unit, 'object': obj.name,
                    'sha256': hashlib.sha256(obj.read_bytes()).hexdigest(), 'command': cmd})
subprocess.run([a.cc, '-nostdlib', '-r', *[str(build / r['object']) for r in records],
                '-o', str(build / 'i915_components.o')], check=True)
subprocess.run(['nm', '-u', str(build / 'i915_components.o')], check=True)
records.append({'object': 'i915_components.o',
                'sha256': hashlib.sha256((build / 'i915_components.o').read_bytes()).hexdigest()})
# Compile the native atomic branch for a separate host-executable CPU probe.
# It calls no kernel APIs; do not link test entry points into the component set.
probe = build / 'native_atomic_probe.o'
cmd = [a.cc, *flags, '-MMD', '-c', str(root / 'tests/native_atomic_probe.c'), '-o', str(probe)]
print(' '.join(cmd), flush=True)
subprocess.run(cmd, check=True)
records.append({'source': 'tests/native_atomic_probe.c', 'object': probe.name,
                'sha256': hashlib.sha256(probe.read_bytes()).hexdigest(),
                'command': cmd, 'componentLink': False})
(build / 'manifest.json').write_text(json.dumps({'sourceRevision': revision,
    'evidenceClass': 'cross-source compilation only; no native runtime',
    'compiler': subprocess.check_output([a.cc, '--version'], text=True).splitlines()[0],
    'objects': records}, indent=2) + '\n')
