#!/usr/bin/env python3
import pathlib,subprocess,sys,os,json,hashlib
root=pathlib.Path(__file__).resolve().parents[2];out=pathlib.Path(sys.argv[1]).resolve();out.mkdir(parents=True,exist_ok=True)
source=out/'composed';source.mkdir(exist_ok=True)
repo='/home/dan/projects/rondoflow-codex/.tools/renku-mission/source-cache/renkuos-pinned.git';rev='4799bb5a188c4bb6bbef967287640cc21a061a0d';results=[]
def run(cmd,cwd=None):
 p=subprocess.run(list(map(str,cmd)),cwd=cwd,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=20);results.append(dict(command=list(map(str,cmd)),exitCode=p.returncode,output=p.stdout));assert p.returncode==0,p.stdout
paths=['src/add-ons/kernel/bus_managers/acpi/'+x for x in ['BusManager.cpp','ACPIPrivate.h','ACPICAHaiku.cpp','Module.cpp']]+['src/system/kernel/module.cpp','src/add-ons/kernel/generic/dpc/dpc.c','headers/os/drivers/dpc.h']
for n in paths:
 p=source/n;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(subprocess.check_output(['git','--git-dir='+repo,'show',rev+':'+n]))
for patch in sorted((root/'patches').glob('01*.patch')):run(['git','apply','--check',patch],source);run(['git','apply',patch],source)
bus=(source/paths[0]).read_text();(out/'production.inc').write_text(bus[bus.index('// Image-owned storage:'):bus.index('static bool\ncheckAndLogFailure')])
exe=(source/paths[2]).read_text();assert 'acpi_queue_callback(function, context)' in exe
run(['g++','-std=c++11','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(out),root/'tests/acpi-callback-host/harness.cpp','-o',out/'test'])
for i in range(8):run([out/'test',str(i)])
run(['python3',root/'tests/dpc-ownership-host/run.py',source/paths[-2],out/'dpc'])
(out/'results.json').write_text(json.dumps(dict(passed=True,hostOnly=True,leakDetection=False,sourceRevision=rev,results=results,sourceHashes={n:hashlib.sha256((source/n).read_bytes()).hexdigest() for n in paths}),indent=2)+'\n')
print('8 callback custody schedules and DPC regression passed; ASan/UBSan, detect_leaks=0')
