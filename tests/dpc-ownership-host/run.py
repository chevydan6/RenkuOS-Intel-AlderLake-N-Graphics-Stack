#!/usr/bin/env python3
import json, os, pathlib, subprocess, sys
root=pathlib.Path(__file__).resolve().parents[2]
source=pathlib.Path(sys.argv[1]).resolve()
out=pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True,exist_ok=True)
cmd=['cc','-std=gnu11','-Wall','-Wextra','-Werror','-pthread','-fsanitize=address,undefined','-fno-omit-frame-pointer','-g','-no-pie','-I'+str(source.parents[5]/'headers/os/drivers'),'-I'+str(root/'tests/dpc-ownership-host'),'-DDPC_SOURCE="'+str(source)+'"',str(root/'tests/dpc-ownership-host/test.c'),'-o',str(out/'test')]
results=[]
def run(cmd):
 p=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=8,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 results.append(dict(command=cmd,exitCode=p.returncode,output=p.stdout)); return p.returncode
ok=run(cmd)==0
if ok:
 for case in ['drain','full','wake','wake-close','start','spawn-cleanup','sem','join','exit','executing','requeue','self','acquire']:
  if run([str(out/'test'),case]): ok=False
(out/'results.json').write_text(json.dumps({'passed':ok,'leakDetection':False,'limitation':'LeakSanitizer disabled due to known traced restriction','results':results},indent=2)+'\n')
print(json.dumps({'passed':ok,'cases':len(results)-1})); sys.exit(0 if ok else 1)
