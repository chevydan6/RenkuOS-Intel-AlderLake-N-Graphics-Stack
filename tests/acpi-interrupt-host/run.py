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
for name in ['0101-acpi-dpc-admission.patch','0111-acpi-callback-custody.patch','0112-acpi-interrupt-identity.patch']:
 patch=(root/'patches'/name).read_text()
 sections=patch.split('--- a/')
 selected=''.join('--- a/'+x for x in sections[1:] if x.startswith(p+'\n'))
 subprocess.run(['git','apply'],input=selected,text=True,cwd=composed,check=True)
s=target.read_text()
records=s[s.index('// Identity stays reserved'):s.index('#endif',s.index('// Identity stays reserved'))]
a=s.index('UINT32\nAcpiOsInstallInterruptHandler(');b=s.index('/******************************************************************************',s.index('AcpiOsRemoveInterruptHandler(UINT32'))
(out/'production.inc').write_text(records+s[a:b])
cmd=['g++','-std=c++11','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie','-g','-I'+str(out),str(root/'tests/acpi-interrupt-host/harness.cpp'),'-o',str(out/'test')]
r=subprocess.run(cmd,capture_output=True,text=True);assert r.returncode==0,r.stderr
r=subprocess.run([str(out/'test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},capture_output=True,text=True,timeout=15)
(out/'results.json').write_text(json.dumps(dict(passed=r.returncode==0,command=cmd,output=r.stdout+r.stderr,exitCode=r.returncode,sourceSha256=hashlib.sha256(target.read_bytes()).hexdigest(),hostOnly=True,leakDetection=False),indent=2)+'\n');assert r.returncode==0,r.stderr
print(r.stdout)
