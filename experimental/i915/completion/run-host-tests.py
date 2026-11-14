#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import argparse
import os
from pathlib import Path
import resource
import json
import shlex
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = Path(__file__).resolve().parents[1]
subprocess.run(['python3', str(root/'extract-folio.py')], check=True)
subprocess.run(['python3', str(root/'verify-upstream.py')], check=True)
subprocess.run(['python3', str(root/'extract-bitmap.py')], check=True)
subprocess.run(['python3', str(root/'extract-gfp.py')], check=True)
subprocess.run(['python3', str(root/'extract-highmem.py')], check=True)
original = (root/'upstream/kernel/sched/completion.c').read_text()
copy = a.output/'completion.c'
copy.write_text(original)
find_original = (root/'upstream/lib/find_bit.c').read_text()
find_copy = a.output/'find_bit.c'
find_copy.write_text(find_original)
equal_original = (root/'build/bitmap_equal_slice.c').read_text()
equal_copy = a.output/'bitmap_equal_slice.c'
equal_copy.write_text(equal_original)
lockref_original = (root/'upstream/lib/lockref.c').read_text()
lockref_copy = a.output/'lockref.c'
lockref_copy.write_text(lockref_original)
seq_original = (root/'upstream/linux/seqlock.h').read_text()
seq_copy = a.output/'seqlock.h'; seq_copy.write_text(seq_original)
seq_include = a.output/'include/linux'; seq_include.mkdir(parents=True)
wrapper = (root/'include/linux/seqlock.h').read_text()
assert wrapper.count('../../upstream/linux/seqlock.h') == 1
(seq_include/'seqlock.h').write_text(wrapper.replace('../../upstream/linux/seqlock.h', str(seq_copy.resolve())))
pages_original = (root/'src/native_pages.c').read_text()
pages_copy = a.output/'native_pages.c'; pages_copy.write_text(pages_original)
flags = ['gcc', '-I'+str(a.output/'include'), '-D_GNU_SOURCE', '-std=gnu11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-Wno-unused-parameter', '-Wno-sign-compare', '-fno-strict-overflow', '-pthread',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
         '-I'+str(root/'include'), '-I'+str(root/'upstream'), '-I'+str(root/'completion')]
objects = []
def build():
    units = [copy, root/'upstream/kernel/sched/swait.c', root/'src/completion_sched.c',
             root/'src/task_identity.c', root/'src/printk_bridge.c', root/'src/time.c', root/'completion/host_backend.c',
             root/'completion/host_test.c', root/'src/cpumask_state.c', equal_copy, find_copy, root/'upstream/lib/hweight.c', lockref_copy, pages_copy, root/'src/native_shmem.c', root/'tests/page_bridge_host.c', root/'upstream/lib/refcount.c']
    log = ''
    for i, source in enumerate(units):
        obj = a.output/(str(i)+'.o')
        command = flags.copy()
        if source.name == 'find_bit.c': command += ['-Wno-error=maybe-uninitialized']
        if i < 2: command += ['-include', str(root/'include/completion_port.h')]
        command += ['-c', str(source), '-o', str(obj)]
        r = subprocess.run(command, capture_output=True, text=True, timeout=30)
        log += ' '.join(command)+'\n'+r.stdout+r.stderr
        (a.output/'build.log').write_text(log)
        r.check_returncode()
        if obj not in objects: objects.append(obj)
    result = subprocess.run(flags+list(map(str, objects))+shlex.split(os.environ.get('SANITIZER_LDFLAGS', ''))+
                   ['-o', str(a.output/'test')], text=True, capture_output=True, timeout=30)
    (a.output/'link.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'run.log').write_text(r.stdout+r.stderr)
r.check_returncode()
# The canonical helpers must still reject broken assumptions and mixed types.
controls = []
for name, body, should_pass in [
    ('helper_positive', 'BUILD_BUG_ON(sizeof(unsigned long) != 8); return min(3, 7);', True),
    ('build_bug_rejected', 'BUILD_BUG_ON(1); return 0;', False),
    ('invalid_genmask_rejected', 'return GENMASK(2, 3);', False),
    ('mixed_min_rejected', 'int a = 1; unsigned long b = 2; return min(a, b);', False),
]:
    source = a.output/(name+'.c')
    source.write_text('#include <linux/minmax.h>\n#include <linux/build_bug.h>\nint check(void) {'+body+'}\n')
    cmd = flags+['-c', str(source), '-o', str(a.output/(name+'.o'))]
    result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text(result.stdout+result.stderr)
    assert (result.returncode == 0) == should_pass
    if not should_pass: assert 'error:' in result.stderr
    controls.append({'name': name, 'exit': result.returncode})
(a.output/'helper-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
before = 'x->done--;'
assert original.count(before) == 2
copy.write_text(original.replace(before, 'x->done = 0;'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and ('Assertion' in r.stderr
    or 'i915 completion test worker lifetime failure' in r.stderr)
copy.write_text(original)
# A search that ignores the first-word offset must be rejected by the oracle.
before_find = 'mask = MUNGE(BITMAP_FIRST_WORD_MASK(__start));'
assert find_original.count(before_find) == 1
find_copy.write_text(find_original.replace(before_find, 'mask = ~0UL;'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'find-negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
find_copy.write_text(find_original)
before_equal = '& BITMAP_LAST_WORD_MASK(bits)'
assert equal_original.count(before_equal) == 1
equal_copy.write_text(equal_original.replace(before_equal, '& ~0UL'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'equal-negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
equal_copy.write_text(equal_original)
build()
print('PASS original completion/swait under ASan/UBSan; lost completion-count regression rejected')

r = subprocess.run([str(a.output/'test'), 'unlocked-mark-dead'], capture_output=True, text=True, timeout=30)
(a.output/'lockref-context-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'assertion' in r.stderr
# Missing dead-reference rejection must fail the behavioral oracle.
before_lockref = 'if (lockref->count >= 0) {'
assert lockref_original.count(before_lockref) == 1
lockref_copy.write_text(lockref_original.replace(before_lockref, 'if (1) {'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'lockref-negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
lockref_copy.write_text(lockref_original); build()
print('PASS original lockref fallback, real host concurrency and rejected dead-reference mutation')

# A retry implementation that always accepts an old snapshot must be rejected.
before_seq = 'return unlikely(READ_ONCE(s->sequence) != start);'
assert seq_original.count(before_seq) == 1
seq_copy.write_text(seq_original.replace(before_seq, 'return 0;'))
build()
r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
(a.output/'seq-negative-control.log').write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
assert r.returncode == -6 and 'Assertion' in r.stderr
seq_copy.write_text(seq_original); build()
print('PASS original seqcount/seqlock, concurrent snapshots and rejected always-accept retry mutation')

for name, before, after in [
    ('page-missing-reference-release', 'bool last = refcount_dec_and_test(&run->references);', 'bool last = false;'),
    ('page-missing-native-delete', 'if (renku_vm_page_run_delete(run->area) != 0) Invalid("native area deletion failed");', '(void)run->area;'),
]:
    assert pages_original.count(before) == 1
    pages_copy.write_text(pages_original.replace(before, after)); build()
    r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
    assert r.returncode == -6 and 'Assertion' in r.stderr
pages_copy.write_text(pages_original); build()
print('PASS managed page references, queued VM release, nesting/failure controls; omitted reference release/native deletion rejected')

for name, before, after in [
    ('folio-tail-redirect', 'struct folio *folio = run ? (struct folio *)run->pages : NULL;',
     'struct folio *folio = run ? (struct folio *)page : NULL;'),
    ('folio-batch-drop', 'for (unsigned i = 0; i < batch->nr; i++) folio_put(batch->folios[i]);',
     '/* lost batch reference release */'),
]:
    assert pages_original.count(before) == 1
    pages_copy.write_text(pages_original.replace(before, after)); build()
    r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
    assert r.returncode == -6 and ('Assertion' in r.stderr
        or 'i915 completion test worker lifetime failure' in r.stderr)
pages_copy.write_text(pages_original); build()
print('PASS canonical folio heads, original local mappings and real batch reference release; tail-redirection/lost-batch mutations rejected')

for name, before, after in [
    ('store-missing-unpin', 'if (renku_vm_page_store_unpin(run->base) != 0) Invalid("native store unpin failed");',
     '(void)run->base;'),
    ('store-duplicate-pin-leak', 'if (existing && renku_vm_page_store_unpin(run->base) != 0) Invalid("duplicate store pin release failed");',
     '(void)existing;'),
    ('store-duplicate-head', 'existing = FindStorePage(store, index);\n\t\tif (existing)',
     'existing = NULL;\n\t\tif (existing)'),
    ('store-delete-live', 'if (store->pins || store->in_flight)', 'if (store->in_flight)'),
]:
    assert pages_original.count(before) == 1
    pages_copy.write_text(pages_original.replace(before, after)); build()
    r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text('exit='+str(r.returncode)+'\n'+r.stdout+r.stderr)
    assert r.returncode == -6 and 'Assertion' in r.stderr
pages_copy.write_text(pages_original); build()
print('PASS persistent-store pin lifetime and concurrent canonical heads; missing unpins, duplicate heads and live deletion rejected')
