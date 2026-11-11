#!/usr/bin/env python3
"""Production owner export plus ACPI entry; mocked kernel/module services only."""
import pathlib, subprocess, sys, json, os, hashlib
root=pathlib.Path(__file__).resolve().parents[2]
out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
subprocess.run(['python3',str(root/'tests/acpi-dmar-host/run.py'),str(out/'provider')],check=True)
source=pathlib.Path(sys.argv[2]).resolve()
owner=(source/'src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp').read_text()
# Use the complete composed production header, including later lease fixes.
(out/'IntelApertureLeaseState.h').write_bytes((source/'headers/private/graphics/intel_extreme/IntelApertureLeaseState.h').read_bytes())
def fun(key):
 a=owner.index(key);return owner[a:owner.index('\n}\n',a)+3]
api=(root/'patches/0097-intel-memory-dmar-preflight.patch').read_text().split('+++ b/headers/private/graphics/intel_extreme/IntelMemoryOwner.h\n',1)[1].split('--- /dev/null',1)[0]
api='\n'.join(x[1:] for x in api.splitlines() if x.startswith('+') and not x.startswith('+++'))
api=api[api.index('// Separate'):]
globals=owner[owner.index('// Stable resident storage;'):owner.index('static status_t\nStatus(')]
export=owner[owner.index('static intel_memory_preflight_module_info sPreflightModule'):owner.index('\nmodule_info* modules[]')]
(out/'owner.inc').write_text(api+'\n'+globals+fun('static status_t\nStatus(')+fun('static status_t\nReleaseDisplay(')+fun('static status_t\nPreflightDMAR(')+fun('static status_t\nObservationOperations(')+fun('static status_t\nStandardOperations(')+export)
base=(root/'tests/acpi-dmar-host/harness.cpp').read_text().split('int main(',1)[0]
base=base.replace('int get_module(', 'int core_get_module(').replace('int put_module(', 'int core_put_module(')
base=base.replace('#include "production.inc"','#define get_module core_get_module\n#define put_module core_put_module\n#include "provider/production.inc"\n#undef get_module\n#undef put_module')
for name in ['IntelN100PciPower.h']:
 (out/name).write_bytes((source/'headers/private/graphics/intel_extreme'/name).read_bytes())
(out/'admission.inc').write_text(fun('static void\nRetireObservation(')+fun('static status_t\nBeginObservationPhase('))
extra=(root/'tests/owner-dmar-host/harness.inc').read_text()
(out/'test.cpp').write_text(base+extra)
cmd=['g++','-std=c++17','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-I'+str(out),str(out/'test.cpp'),'-o',str(out/'test')]
subprocess.run(cmd,check=True)
results=[]
for case in ['empty','unique','duplicate','not-ready','mutex','dependency','nested','observation','close','repeated','preflight-first','observation-first','pci-failure','gart-failure']:
 p=subprocess.run([str(out/'test'),case],capture_output=True,text=True,timeout=10,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 results.append(dict(case=case,exit=p.returncode,output=p.stdout+p.stderr));assert p.returncode==0,p.stderr
(out/'results.json').write_text(json.dumps(dict(cases=results,command=cmd,hostOnly=True,leakDetection=False,ownerSha256=hashlib.sha256(owner.encode()).hexdigest()),indent=2)+'\n')
print('14 owner-export/ACPI integration schedules passed under ASan/UBSan')
