#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Build an isolated native owner candidate from the exact installed baseline."""
import argparse,hashlib,pathlib,shutil,subprocess,tarfile
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--owner',type=pathlib.Path,required=True)
p.add_argument('--output',type=pathlib.Path,required=True)
p.add_argument('--headers',type=pathlib.Path,help='verified fallback for sparse-checkout headers')
a=p.parse_args();here=pathlib.Path(__file__).resolve().parent;repo=here.parents[1]
raw=a.owner.read_bytes()
expected='bb097546e9e6fa8f0cf18fcd173c0d959ce208c35b9087f8ee4ad8f0b661b894'
if hashlib.sha256(raw).hexdigest()!=expected:raise SystemExit('Owner baseline mismatch')
out=a.output;out.mkdir(exist_ok=False,parents=True)
for path in here.glob('*'):
 if path.is_file() and path.suffix in ('.h','.inc','.cpp','.sh'):shutil.copy2(path,out/path.name)
shutil.copytree(here/'render',out/'render',ignore=shutil.ignore_patterns('__pycache__'))
tracked=subprocess.check_output(['git','ls-tree','-r','HEAD','workstreams/gpu-next'],cwd=repo,text=True)
objects={pathlib.Path(name).name:metadata.split()[2]
 for metadata,name in (line.split('\t',1) for line in tracked.splitlines()) if name.endswith('.h')}
headers={path.name for path in (repo/'workstreams/gpu-next').glob('*.h')}
if a.headers:headers.update(path.name for path in a.headers.glob('Intel*.h'))
required={'IntelDmarInventory.h','IntelDmarReserved.h','IntelN100ContextImage.h',
 'IntelN100ContextStatus.h','IntelN100CsbReader.h','IntelN100NonceStores.h',
 'IntelN100Ppgtt.h','IntelN100PpgttTree.h','IntelVtdObservation.h'}
if not required<=headers:raise SystemExit(f'Missing native owner headers: {sorted(required-headers)}')
for name in sorted(headers):
 source=repo/'workstreams/gpu-next'/name
 if source.exists():shutil.copy2(source,out/name)
 else:
  data=(a.headers/name).read_bytes()
  object_id=objects.get(name)
  if object_id and hashlib.sha1(f'blob {len(data)}\0'.encode()+data).hexdigest()!=object_id:
   raise SystemExit(f'Header provenance mismatch: {name}')
  (out/name).write_bytes(data)
for name in ('LinearCopy.h','LICENSE.intel'):
 source=repo/'experimental/gen12'/name
 if source.exists():shutil.copy2(source,out/name)
 else:(out/name).write_bytes(subprocess.check_output(
  ['git','show',f'HEAD:experimental/gen12/{name}'],cwd=repo))
(out/'include').mkdir();(out/'src').mkdir()
for source,dest in [('include/native_page_bridge.h','include/native_page_bridge.h'),('src/native_page_bridge.cpp','src/native_page_bridge.cpp')]:
 shutil.copy2(repo/'experimental/i915'/source,out/dest)
anchor='// v1 acquires v0 through the module manager.'
text=raw.decode();assert text.count(anchor)==1
hook='static status_t\nReleaseDisplay(uint64 lease)\n{\n'
assert text.count(hook)==1
text=text.replace(hook,'static status_t NativeRcsDisplayClosing(uint64_t lease);\n'+hook+'\tstatus_t nativeClose=NativeRcsDisplayClosing(lease);\n\tif(nativeClose!=B_OK)return nativeClose;\n')
hook='static bool\nResetOwnerPower(GroupContext& context)\n{\n'
assert text.count(hook)==1
text=text.replace(hook,'static bool NativeOwnerPower(GroupContext& context);\n'+hook+'\tif(sNativeExecutionActive)return NativeOwnerPower(context);\n')
hook='static bool\nHandshakePciStable(void* data)\n{\n'
assert text.count(hook)==1
text=text.replace(hook,'static bool sNativeExecutionActive;\nstatic bool NativeHandshakePciStable(void* data);\n'+hook+'\tif(sNativeExecutionActive)return NativeHandshakePciStable(data);\n')
text=text.replace(anchor,'static status_t ObservationOperations(int32 operation, ...);\n#include "NativeExecution.inc"\n\n'+anchor)
anchor='module_info* modules[] = {\n';assert text.count(anchor)==1
text=text.replace(anchor,anchor+'\t(module_info*)&sNativeExecutionModule,\n\t(module_info*)&sNativeRenderModule,\n\t(module_info*)&sNativeResourceModule,\n')
(out/'IntelMemoryOwner-execution.cpp').write_text(text)
(out/'baseline-owner.sha256').write_text(expected+'  /boot/home/i915-mapping-002/source/src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp\n')
(out/'payload.sha256').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+str(f.relative_to(out))+'\n' for f in sorted(out.rglob('*')) if f.is_file()))
with tarfile.open(str(out)+'.tar.gz','w:gz') as t:
 for f in sorted(out.rglob('*')):
  if f.is_file():t.add(f,arcname=str(f.relative_to(out)))
print(str(out)+'.tar.gz')
