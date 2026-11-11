#!/usr/bin/env python3
import pathlib, subprocess, os, sys, json, hashlib
root=pathlib.Path(__file__).resolve().parents[2]
out=pathlib.Path(sys.argv[1]).resolve(); out.mkdir(parents=True,exist_ok=True)
repo='/home/dan/projects/rondoflow-codex/.tools/renku-mission/source-cache/renkuos-pinned.git'
rev='4799bb5a188c4bb6bbef967287640cc21a061a0d'
p='src/add-ons/kernel/bus_managers/acpi/ACPICAHaiku.cpp'
s=subprocess.check_output(['git','--git-dir='+repo,'show',rev+':'+p])
composed=out/'composed'; target=composed/p; target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(s)
# Only the patches that touch this file; other files are checked by regression.
for name in ['0101-acpi-dpc-admission.patch','0111-acpi-callback-custody.patch','0112-acpi-interrupt-identity.patch','0113-acpi-interrupt-admission.patch']:
 patch=(root/'patches'/name).read_text()
 sections=patch.split('--- a/')
 selected=''.join('--- a/'+x for x in sections[1:] if x.startswith(p+'\n'))
 subprocess.run(['git','apply'],input=selected,text=True,cwd=composed,check=True)
# Compose lifecycle caller and private declaration as well.
for filename in ['BusManager.cpp', 'ACPIPrivate.h']:
 path='src/add-ons/kernel/bus_managers/acpi/'+filename
 dest=composed/path; dest.write_bytes(subprocess.check_output(['git','--git-dir='+repo,'show',rev+':'+path]))
 for name in ['0101-acpi-dpc-admission.patch','0104-acpi-lifecycle-retention.patch','0111-acpi-callback-custody.patch','0113-acpi-interrupt-admission.patch']:
  patch=(root/'patches'/name).read_text()
  selected=''.join('--- a/'+x for x in patch.split('--- a/')[1:] if x.startswith(path+'\n'))
  if selected: subprocess.run(['git','apply'],input=selected,text=True,cwd=composed,check=True)
bus=(composed/'src/add-ons/kernel/bus_managers/acpi/BusManager.cpp').read_text()
a=bus.index('static status_t\nacpi_retain_lifecycle(');b=bus.index('\n}\n',a)+3
(out/'lifecycle.inc').write_text(bus[a:b])
assert 'void acpi_close_interrupt_admission();' in (composed/'src/add-ons/kernel/bus_managers/acpi/ACPIPrivate.h').read_text()
s=target.read_text()
records=s[s.index('// Identity stays reserved'):s.index('#endif',s.index('// Identity stays reserved'))]
a=s.index('UINT32\nAcpiOsInstallInterruptHandler(');b=s.index('/******************************************************************************',s.index('AcpiOsRemoveInterruptHandler(UINT32'))
(out/'production.inc').write_text(records+s[a:b])
cmd=['g++','-std=c++11','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-g','-I'+str(out),str(root/'tests/acpi-interrupt-host/harness.cpp'),'-o',str(out/'test')]
r=subprocess.run(cmd,capture_output=True,text=True);assert r.returncode==0,r.stderr
r=subprocess.run([str(out/'test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},capture_output=True,text=True,timeout=15)
(out/'results.json').write_text(json.dumps(dict(passed=r.returncode==0,command=cmd,output=r.stdout+r.stderr,exitCode=r.returncode,sourceSha256=hashlib.sha256(target.read_bytes()).hexdigest(),hostOnly=True,leakDetection=False),indent=2)+'\n');assert r.returncode==0,r.stderr
assert r.returncode==0
for case in ['closed','late-success','late-failure','removing']:
 r=subprocess.run([str(out/'test'),case],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},capture_output=True,text=True,timeout=15)
 assert r.returncode==0,r.stdout+r.stderr
 print(case+': '+r.stdout.strip())
(out/'closure-results.json').write_text(json.dumps(dict(passed=True,cases=['closed','late-success','late-failure','removing'],hostOnly=True,leakDetection=False,limitation='Known traced LeakSanitizer restriction; no GPU or native kernel execution proof',sha256={str(p.relative_to(composed)):hashlib.sha256(p.read_bytes()).hexdigest() for p in composed.rglob('*') if p.is_file()}),indent=2)+'\n')
