#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build an isolated native owner candidate from the exact installed baseline."""
import argparse,hashlib,pathlib,shutil,tarfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--owner',type=pathlib.Path,required=True)
p.add_argument('--output',type=pathlib.Path,required=True)
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent;repo=here.parents[1]
raw=a.owner.read_bytes()
expected='bb097546e9e6fa8f0cf18fcd173c0d959ce208c35b9087f8ee4ad8f0b661b894'
if hashlib.sha256(raw).hexdigest()!=expected:raise SystemExit('Owner baseline mismatch')
out=a.output;out.mkdir(exist_ok=False,parents=True)
for path in here.glob('*'):
 if path.is_file() and path.suffix in ('.h','.inc','.cpp','.sh'):shutil.copy2(path,out/path.name)
for path in (repo/'workstreams/gpu-next').glob('*.h'):shutil.copy2(path,out/path.name)
shutil.copy2(repo/'experimental/gen12/LinearCopy.h',out/'LinearCopy.h')
shutil.copy2(repo/'experimental/gen12/LICENSE.intel',out/'LICENSE.intel')
(out/'include').mkdir();(out/'src').mkdir()
for source,dest in [('include/native_page_bridge.h','include/native_page_bridge.h'),('src/native_page_bridge.cpp','src/native_page_bridge.cpp')]:
 shutil.copy2(repo/'experimental/i915'/source,out/dest)
anchor='// v1 acquires v0 through the module manager.'
text=raw.decode();assert text.count(anchor)==1
hook='static bool\nResetOwnerPower(GroupContext& context)\n{\n'
assert text.count(hook)==1
text=text.replace(hook,'static bool NativeOwnerPower(GroupContext& context);\n'+hook+'\tif(sNativeExecutionActive)return NativeOwnerPower(context);\n')
hook='static bool\nHandshakePciStable(void* data)\n{\n'
assert text.count(hook)==1
text=text.replace(hook,'static bool sNativeExecutionActive;\nstatic bool NativeHandshakePciStable(void* data);\n'+hook+'\tif(sNativeExecutionActive)return NativeHandshakePciStable(data);\n')
text=text.replace(anchor,'static status_t ObservationOperations(int32 operation, ...);\n#include "NativeExecution.inc"\n\n'+anchor)
anchor='module_info* modules[] = {\n';assert text.count(anchor)==1
text=text.replace(anchor,anchor+'\t(module_info*)&sNativeExecutionModule,\n')
(out/'IntelMemoryOwner-execution.cpp').write_text(text)
(out/'baseline-owner.sha256').write_text(expected+'  /boot/home/i915-mapping-002/source/src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp\n')
(out/'payload.sha256').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(out))+'\n' for f in sorted(out.rglob('*')) if f.is_file()))
with tarfile.open(str(out)+'.tar.gz','w:gz') as t:
 for f in sorted(out.rglob('*')):
  if f.is_file():t.add(f,arcname=str(f.relative_to(out)))
print(str(out)+'.tar.gz')
