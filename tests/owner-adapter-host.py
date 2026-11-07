#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile unchanged adapter functions against controlled kernel/GART mocks."""
import argparse, hashlib, json, pathlib, resource, shutil, subprocess
p = argparse.ArgumentParser()
p.add_argument('--source', type=pathlib.Path, required=True)
p.add_argument('--output', type=pathlib.Path, required=True)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
source = a.source / 'src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp'
header = a.source / 'headers/private/graphics/intel_extreme/IntelApertureLeaseState.h'
text = source.read_text()
start = text.index('static status_t\nStatus(')
end = text.index('static status_t\nBeginDisplay(')
(a.output / 'adapter-under-test.inc').write_text(text[start:end])
shutil.copy2(header, a.output / header.name)
shutil.copy2(pathlib.Path(__file__).with_name('owner-adapter-test.cpp'), a.output / 'owner-adapter-test.cpp')
command = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-pthread', '-I', str(a.output), str(a.output / 'owner-adapter-test.cpp'), '-o', str(a.output / 'test')]
r = subprocess.run(command, capture_output=True, text=True, timeout=30)
(a.output / 'build.log').write_text(r.stdout + r.stderr)
r.check_returncode()
r = subprocess.run([str(a.output / 'test')], capture_output=True, text=True, timeout=30)
(a.output / 'test.log').write_text(r.stdout + r.stderr)
(a.output / 'result.json').write_text(json.dumps(dict(exit=r.returncode, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(), header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(), scope='Actual Status/Operation/Query/Allocate/Free/FinishDisplay/ReleaseDisplay; mocked kernel mutex and GART. BeginDisplay/module initialization not exercised.'), indent=2)+'\n')
print(r.stdout, end='')
r.check_returncode()
# Mutate only generated copies, never the production adapter. Each control
# must reach an assertion failure rather than merely fail compilation.
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
original = text[start:end]
controls = []
for name, before, after in [
    ('lost_failure', 'fCanRetainBacking && fFailed', 'false'),
    ('lost_success', 'operation.Succeeded();', '(void)0;'),
]:
    assert before in original
    (a.output / 'adapter-under-test.inc').write_text(original.replace(before, after))
    subprocess.run(command, check=True, capture_output=True, timeout=30)
    control = subprocess.run([str(a.output / 'test')], capture_output=True,
                             text=True, timeout=30)
    controls.append(dict(name=name, exit=control.returncode, stderr=control.stderr))
    assert control.returncode == -6 and 'Assertion' in control.stderr
(a.output / 'negative-controls.json').write_text(json.dumps(controls, indent=2)+'\n')
(a.output / 'adapter-under-test.inc').write_text(original)
subprocess.run(command, check=True, capture_output=True, timeout=30)
print('Two generated adapter regressions rejected by assertions.')
