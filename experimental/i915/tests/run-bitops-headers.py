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
           'linux/kdev_t.h', 'uapi/linux/kdev_t.h', 'linux/gfp.h', 'linux/gfp_types.h', 'linux/lockref.h',
           'linux/seqlock.h', 'linux/kcsan-checks.h', 'asm/processor.h',
           'asm/barrier.h', 'linux/compiler.h', 'linux/mm.h', 'linux/highmem.h',
           'linux/uaccess.h', 'native_pages.h', 'native_page_bridge.h']
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

# Scope checks run after a successful lockref include, catching leaked guards.
controls = []
for mode, flags in [('host', host), ('target', native)]:
    for name, prefix, suffix in [
        ('vm_bounds_rejected', '', '#include <generated/bounds.h>\n'),
        ('cmpxchg_rejected', '#define CONFIG_ARCH_USE_CMPXCHG_LOCKREF 1\n', ''),
        ('kcsan_rejected', '#define CONFIG_KCSAN 1\n', '#include <linux/seqlock.h>\n'),
        ('rt_rejected', '#define CONFIG_PREEMPT_RT 1\n', '#include <linux/seqlock.h>\n'),
        ('highmem_rejected', '#define CONFIG_HIGHMEM 1\n', '#include <linux/highmem.h>\n'),
        ('local_slots_rejected', '#define CONFIG_KMAP_LOCAL 1\n', '#include <linux/highmem.h>\n'),
        ('unmap_page_type_rejected', '', '#include <linux/highmem.h>\nvoid bad(struct page *p) {kunmap_atomic(p); }\n'),
    ]:
        source = a.output/(mode+'-'+name+'.c')
        source.write_text(prefix+'#include <linux/lockref.h>\n'+suffix)
        r = subprocess.run(flags+['-c', str(source), '-o', str(source.with_suffix('.o'))],
                           capture_output=True, text=True, timeout=30)
        source.with_suffix('.log').write_text(r.stdout+r.stderr)
        assert r.returncode != 0 and 'error:' in r.stderr
        controls.append({'mode': mode, 'control': name, 'exit': r.returncode})
(a.output/'scope-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
print('PASS VM bounds and unsupported cmpxchg branch remain rejected in both modes')

# BH operations must remain unresolved rather than acquiring an IRQ-only lock.
for mode, flags in [('host', host), ('target', native)]:
    source = a.output/(mode+'-bh-unresolved.c')
    source.write_text('#include <linux/seqlock.h>\nvoid check(seqlock_t *s) {write_seqlock_bh(s);write_sequnlock_bh(s); }\n')
    obj = source.with_suffix('.o')
    r = subprocess.run(flags+['-fPIC', '-c', str(source), '-o', str(obj)], capture_output=True, text=True)
    assert r.returncode == 0, r.stderr
    symbols = subprocess.check_output(['nm', '-u', str(obj)], text=True)
    assert 'renku_linux_spin_lock_bh' in symbols and 'renku_linux_spin_unlock_bh' in symbols
    r = subprocess.run(['gcc', '-shared', '-nostdlib', '-Wl,--no-undefined', str(obj),
                        '-o', str(obj.with_suffix('.so'))], capture_output=True, text=True)
    source.with_suffix('.log').write_text(symbols+r.stdout+r.stderr)
    assert r.returncode != 0 and 'renku_linux_spin_lock_bh' in r.stderr
print('PASS unimplemented BH entry/exit fail actual linking in both modes')
