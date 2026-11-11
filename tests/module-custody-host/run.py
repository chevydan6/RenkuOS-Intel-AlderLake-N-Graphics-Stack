#!/usr/bin/env python3
import pathlib,subprocess,sys,os,json,hashlib
root=pathlib.Path(__file__).resolve().parents[2];out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
repo='/home/dan/projects/rondoflow-codex/.tools/renku-mission/source-cache/renkuos-pinned.git';rev='4799bb5a188c4bb6bbef967287640cc21a061a0d'
source=out/'composed';source.mkdir(exist_ok=True); results=[]; hashes={}
def run(args,cwd=None):
 p=subprocess.run(list(map(str,args)),cwd=cwd,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=25)
 results.append(dict(command=list(map(str,args)),exitCode=p.returncode,output=p.stdout));assert p.returncode==0,p.stdout
paths=['src/system/kernel/module.cpp','src/add-ons/kernel/bus_managers/acpi/BusManager.cpp','src/add-ons/kernel/bus_managers/acpi/ACPIPrivate.h','src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp','src/add-ons/kernel/generic/dpc/dpc.c','headers/os/drivers/dpc.h','src/add-ons/kernel/bus_managers/acpi/Module.cpp']
for name in paths:
 data=subprocess.check_output(['git','--git-dir='+repo,'show',rev+':'+name]);p=source/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);hashes[name]={'pinned':hashlib.sha256(data).hexdigest()}
for name in ['0100-dpc-truthful-ownership.patch','0101-acpi-dpc-admission.patch','0102-module-quarantine-entry.patch','0103-image-dependency-custody.patch','0104-acpi-lifecycle-retention.patch','0105-module-lifecycle-custody.patch']:
 run(['git','apply','--check',root/'patches'/name],source);run(['git','apply',root/'patches'/name],source)
for name in paths:hashes[name]['composed']=hashlib.sha256((source/name).read_bytes()).hexdigest()
s=(source/paths[0]).read_text()
def extract(marker):
 start=s.index(marker);body=s.index('{',start);depth=1;i=body+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]+(';' if marker in ('enum dependency_phase ', 'struct module_image ') else '')+'\n'
(out/'production.inc').write_text(extract('enum dependency_phase ')+extract('struct module_image ')+extract('static status_t\nput_dependent_modules(')+extract('static status_t\nget_dependent_modules(')+extract('static inline status_t\ninit_module(')+extract('static inline int\nuninit_module(')+'status_t\n'+extract('get_module(const char* path, module_info** _info)')+'status_t\n'+extract('put_module(const char* path)'))
run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(out),root/'tests/module-custody-host/harness.cpp','-o',out/'test']);run([out/'test'])
bus=(source/paths[1]).read_text()
s=bus
admission=bus[bus.index('// Image-owned storage:'):bus.index('static bool\ncheckAndLogFailure')]
state=bus[bus.index('// Retained lifecycle evidence'):bus.index('static bool\nacpi_stage_failed')]
(out/'lifecycle.inc').write_text(admission+state+extract('static bool\nacpi_stage_failed(')+extract('static status_t\nacpi_retain_lifecycle(')+extract('static status_t\nacpi_std_ops('))
run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(out),root/'tests/acpi-lifecycle-host/harness.cpp','-o',out/'lifecycle'])
for case in range(10): run([out/'lifecycle',str(case)])
s=(source/paths[-1]).read_text()
(out/'root.inc').write_text(extract('static int32\nacpi_module_std_ops('))
run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(out),root/'tests/module-custody-host/nested.cpp','-o',out/'nested'])
for case in range(10): run([out/'nested',str(case)])
s=(source/paths[0]).read_text()
(out/'unload.inc').write_text(extract('static status_t\nunload_module_image('))
run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-no-pie','-I'+str(out),root/'tests/module-custody-host/teardown.cpp','-o',out/'teardown']);run([out/'teardown'])
(out/'results.json').write_text(json.dumps(dict(passed=True,sourceRevision=rev,hostOnly=True,ASAN_OPTIONS='detect_leaks=0',limitation='LeakSanitizer disabled due to known traced-process restriction; no GPU execution proof',sourceHashes=hashes,results=results),indent=2)+'\n');print(results[-1]['output'])
