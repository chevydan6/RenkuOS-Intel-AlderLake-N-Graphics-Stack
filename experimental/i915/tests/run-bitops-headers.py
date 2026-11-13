#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check bit helper headers both first and through the normal base include."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
headers = ['linux/bitops.h', 'linux/bits.h', 'linux/bitmap.h', 'linux/types.h',
           'linux/log2.h', 'linux/build_bug.h', 'linux/minmax.h', 'linux/const.h',
           'asm/bitsperlong.h', 'asm/ioctl.h', 'vdso/bits.h', 'linux/cpumask.h',
           'linux/cache.h', 'asm/cache.h', 'asm/param.h', 'linux/limits.h',
           'uapi/linux/limits.h', 'vdso/limits.h', 'linux/kernel.h',
           'linux/linkage.h', 'linux/threads.h', 'linux/numa.h',
           'linux/spinlock_types_raw.h', 'linux/spinlock_types.h',
           'linux/spinlock.h', 'asm/spinlock_types.h', 'linux/bit_spinlock.h',
           'linux/rcupdate.h', 'linux/printk.h', 'linux/kern_levels.h',
           'linux/kdev_t.h', 'uapi/linux/kdev_t.h']
manifest = json.loads((root/'build/target/manifest.json').read_text())
native = manifest['objects'][0]['command']
native = native[:native.index('-MMD')]
host = ['gcc', '-std=gnu11', '-O2', '-Wall', '-Wextra', '-Werror',
        '-Wno-unused-parameter', '-I'+str(root/'include'), '-I'+str(root/'upstream')]
results = []
for mode, flags in [('host', host), ('target', native)]:
    for i, header in enumerate(headers):
        source = a.output/(mode+str(i)+'.c')
        source.write_text('#include <'+header+'>\n#include <linux/bitmap.h>\n#include <'+header+'>\n'
            '_Static_assert(BITS_PER_LONG == 64, "width");\n'
            'unsigned long check(const unsigned long *p) {return find_next_bit(p, 129, 3);}\n')
        command = flags+['-c', str(source), '-o', str(source.with_suffix('.o'))]
        r = subprocess.run(command, text=True, capture_output=True, timeout=30)
        source.with_suffix('.log').write_text(' '.join(command)+'\n'+r.stdout+r.stderr)
        results.append({'mode': mode, 'header': header, 'exit': r.returncode})
(a.output/'results.json').write_text(json.dumps(results, indent=2)+'\n')
assert all(x['exit'] == 0 for x in results), 'Inspect header controls logs'
print('PASS', len(results), 'first/repeated-include combinations with host and real target headers')
