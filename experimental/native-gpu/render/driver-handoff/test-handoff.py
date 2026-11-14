#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Focused golden patch and compiled optional-callback fallback checks."""
import argparse
import importlib.util
from pathlib import Path
import subprocess
import tempfile

spec = importlib.util.spec_from_file_location('check_handoff', Path(__file__).with_name('check-handoff.py'))
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--source', required=True, type=Path)
args = parser.parse_args()
recipe = Path(__file__).with_name("build-driver.sh").read_text()
assert "python" not in recipe.lower(), "native build must not require Python"
original = args.source.read_text()
try:
    checker.check(original, original)
except ValueError as error:
    assert 'missing display-profile' in str(error)
    print('EXPECTED_PREPATCH_FAILURE: missing display-profile handoff callback')
else:
    raise AssertionError('original source unexpectedly contains handoff')
candidate = checker.prepare.transform(original)
checker.check(original, candidate)
try:
    checker.check(original, candidate.replace('return B_OK;', 'return B_ERROR;', 1))
except ValueError:
    pass
else:
    raise AssertionError('unrelated source edit accepted')
# Compile the exact injected statement block, with observable module API stubs.
harness = r'''
#include <cassert>
#include <cstdint>
#include <cstdarg>
#include <cstddef>
using status_t = int32_t;
constexpr int B_OK=0, B_NOT_SUPPORTED=-1, INTEL_GROUP_ALD=1;
#define B_PRId32 "d"
#define NATIVE_GPU_RENDER_MODULE "private-native-render-v1"
#define NATIVE_GPU_RENDER_PROFILE_N100_DISPLAY_ONLY_V1 1
struct module_info {};
struct native_gpu_render_module { status_t (*acknowledge_display_profile)(uint64_t,uint32_t); };
struct Device { bool ald; bool InGroup(int) { return ald; } };
struct Info { Device device_type; uint64_t memory_owner_lease; };
int gets=0, putCount=0, calls=0, logs=0, loadStatus=0, ackStatus=0;
status_t acknowledge(uint64_t lease,uint32_t profile) {
 assert(lease==99 && profile==1); ++calls; return ackStatus;
}
native_gpu_render_module renderModule={acknowledge};
status_t get_module(const char*, module_info** out) {
 ++gets; if(loadStatus==0) *out=reinterpret_cast<module_info*>(&renderModule); return loadStatus;
}
void put_module(const char*) { ++putCount; }
void dprintf(const char*,...) { ++logs; }
status_t display_handoff(Info& info) {
INJECTION
 return B_OK;
}
void run(int load,int ack,bool callback,bool ald,int expectedGets,int expectedPuts,int expectedCalls,int expectedLogs) {
 gets=putCount=calls=logs=0; loadStatus=load; ackStatus=ack;
 renderModule.acknowledge_display_profile=callback ? acknowledge : nullptr;
 Info info={{ald},99}; assert(display_handoff(info)==B_OK);
 assert(gets==expectedGets && putCount==expectedPuts && calls==expectedCalls && logs==expectedLogs);
}
int main() {
 run(-2,0,true,true,1,0,0,1); // unavailable export preserves display
 run(0,-3,true,true,1,1,1,1); // refused/stale lease preserves display, releases module
 run(0,0,false,true,1,1,0,1); // unavailable callback preserves display
 run(0,0,true,true,1,1,1,0); // successful acknowledgement
 run(0,0,true,false,0,0,0,0); // other hardware never calls
}
'''.replace('INJECTION', checker.prepare.HANDOFF)
with tempfile.TemporaryDirectory(prefix='rcs-driver-handoff-') as directory:
    cpp = Path(directory) / 'handoff.cpp'
    executable = Path(directory) / 'handoff'
    cpp.write_text(harness)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print('DRIVER_HANDOFF_GOLDEN_AND_FALLBACK_PASSED')
