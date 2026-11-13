#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile actual owner mapping functions with real lease state and mocked GART."""
import argparse, pathlib, subprocess, shutil, os, shlex, resource, json
p = argparse.ArgumentParser()
p.add_argument('--source', required=True, type=pathlib.Path)
p.add_argument('--output', required=True, type=pathlib.Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
source = (a.source/'src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp').read_text()
header = (a.source/'headers/private/graphics/intel_extreme/IntelMemoryOwner.h').read_text()
(a.output/'mapping-under-test.inc').write_text(source[source.index('// A separate Request lease'):source.index('// v1 acquires v0')])
api = header[header.index('// Memory-only GGTT sandbox'):header.rindex('#endif')]
(a.output/'mapping-api.inc').write_text(api.replace('module_info info;', 'int info;'))
shutil.copy2(a.source/'headers/private/graphics/intel_extreme/IntelApertureLeaseState.h', a.output)
command = ['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=undefined', '-fno-sanitize-recover=all','-g','-pthread','-I'+str(a.output),str(pathlib.Path(__file__).with_name('host-test.cpp')),'-o',str(a.output/'test')]
command += shlex.split(os.environ.get('SANITIZER_LDFLAGS', ''))
for name, cmd in [('build',command),('run',[str(a.output/'test')])]:
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    (a.output/(name+'.log')).write_text(r.stdout+r.stderr)
    print(r.stdout+r.stderr, end='')
    r.check_returncode()
# Regression controls mutate only generated test copies, never the source tree.
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
actual = (a.output/'mapping-under-test.inc').read_text()
controls = []
for name, before, after in [
    ('wrong_cpu_domain', 'buffer.cpu_aperture = sMappingAliasBase + buffer.ggtt_offset;', 'buffer.cpu_aperture = base;'),
    ('wrong_free_domain', 'status = Free(token, sMappingBacking[i]);', 'status = Free(token, buffer.cpu_aperture);'),
    ('lost_free_record', 'sMappingBacking[i] = 0;\n\t\tbuffer = {};', '(void)0;\n\t\tbuffer = {};'),
]:
    assert before in actual
    (a.output/'mapping-under-test.inc').write_text(actual.replace(before, after))
    subprocess.run(command, check=True, capture_output=True, timeout=30)
    r = subprocess.run([str(a.output/'test')], capture_output=True, text=True, timeout=30)
    controls.append({'name':name, 'exit':r.returncode, 'stderr':r.stderr})
    assert r.returncode == -6 and 'Assertion' in r.stderr
(a.output/'negative-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
(a.output/'mapping-under-test.inc').write_text(actual)
subprocess.run(command, check=True, capture_output=True, timeout=30)
print('PASS three generated address-domain/double-free regressions rejected')
