#!/usr/bin/env python3
import pathlib,subprocess,sys,os,json,hashlib
root=pathlib.Path(__file__).resolve().parents[2];out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
subprocess.run(['python3',str(root/'tests/acpi-table-host/run.py'),str(out/'table')],check=True)
src=out/'table/callback/composed/src/add-ons/kernel/bus_managers/acpi'
bus=(src/'BusManager.cpp').read_text();mod=(src/'Module.cpp').read_text();hdr=(src/'ACPIPrivate.h').read_text()
def fun(s,key):
 a=s.index(key);return s[a:s.index('\n}\n',a)+3]
shared=(root/'patches/0097-intel-memory-dmar-preflight.patch').read_text().split('+++ b/headers/private/graphics/intel_extreme/IntelDmarDescriptor.h\n',1)[1]
api='\n'.join(line[1:] for line in shared.splitlines() if line.startswith('+') and not line.startswith('+++'))
api=api[api.index('#define ACPI_DMAR_DESCRIPTOR_MODULE_NAME'):api.index('#endif')]
code=api+bus[bus.index('enum ACPILifecyclePhase'):bus.index('static uint32 sACPIAttempted')]+fun(bus,'static status_t\nacpi_retain_lifecycle(')+fun(bus,'acpi_dmar_descriptor_result\nacpi_classify_dmar(')+fun(mod,'static int32\nacpi_module_std_ops(')
a=mod.index('static acpi_dmar_descriptor_module_info sDMARDescriptorModule');code+=mod[a:mod.index('\n};',a)+4]
assert '(module_info*)&sDMARDescriptorModule,' in mod
(out/'production.inc').write_text(code)
cmd=['g++','-std=c++11','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-I'+str(out),str(root/'tests/acpi-dmar-host/harness.cpp'),'-o',str(out/'test')]
subprocess.run(cmd,check=True);logs=[]
for case in ['unused','starting','retained','empty','unique0','unique1','unique2','duplicate','mutex','overlap','dependency']:
 p=subprocess.run([str(out/'test'),case],capture_output=True,text=True,timeout=10,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'});logs.append(dict(case=case,exitCode=p.returncode,output=p.stdout+p.stderr));assert p.returncode==0,p.stderr
(out/'results.json').write_text(json.dumps(dict(passed=True,hostOnly=True,leakDetection=False,cases=logs,command=cmd,sourceHashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in src.glob('*')}),indent=2)+'\n')
print('11 exported descriptor schedules passed; host ASan/UBSan only')
