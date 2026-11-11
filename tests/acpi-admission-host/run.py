#!/usr/bin/env python3
"""Apply both production patches, extract actual functions, run bounded host tests."""
import pathlib, subprocess, json, os, sys, hashlib
root=pathlib.Path(__file__).resolve().parents[2]
out=pathlib.Path(sys.argv[1]).resolve(); out.mkdir(parents=True,exist_ok=True)
repo='/home/dan/projects/rondoflow-codex/.tools/renku-mission/source-cache/renkuos-pinned.git'
rev='4799bb5a188c4bb6bbef967287640cc21a061a0d'
paths=['src/add-ons/kernel/bus_managers/acpi/'+n for n in ['BusManager.cpp','ACPIPrivate.h','ACPICAHaiku.cpp']]+['src/add-ons/kernel/generic/dpc/dpc.c','headers/os/drivers/dpc.h']
source=out/'composed'; source.mkdir(exist_ok=True)
results=[]
def run(cmd,cwd=None):
 p=subprocess.run(list(map(str,cmd)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 results.append(dict(command=list(map(str,cmd)),exitCode=p.returncode,output=p.stdout)); assert p.returncode==0,p.stdout
for name in paths:
 p=source/name; p.parent.mkdir(parents=True,exist_ok=True); p.write_bytes(subprocess.check_output(['git','--git-dir='+repo,'show',rev+':'+name]))
for patch in ['0100-dpc-truthful-ownership.patch','0101-acpi-dpc-admission.patch']:
 run(['git','apply','--check',root/'patches'/patch],source)
 run(['git','apply',root/'patches'/patch],source)
bus=(source/paths[0]).read_text(); exe=(source/paths[2]).read_text()
admission=bus[bus.index('// Image-owned storage:'):bus.index('static bool\ncheckAndLogFailure')]
start=exe.index('ACPI_STATUS\nAcpiOsExecute('); end=exe.index('\n}\n',start)+3
(out/'production.inc').write_text(admission+exe[start:end])
run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(out),root/'tests/acpi-admission-host/harness.cpp','-o',out/'test'])
for case in ['success','prelock','leased-close','postclose','wake','reject','ambiguous','null','creating-close','counter','generation']:
 run([out/'test',case])
run(['python3',root/'tests/dpc-ownership-host/run.py',source/paths[3],out/'dpc'])
(out/'results.json').write_text(json.dumps(dict(passed=True,hostOnly=True,leakDetection=False,limitation='ASAN_OPTIONS=detect_leaks=0: known traced LeakSanitizer restriction',results=results,sourceRevision=rev,sha256={p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in paths}),indent=2)+'\n')
print('11 admission cases and 13 DPC cases passed; both patches compose')
