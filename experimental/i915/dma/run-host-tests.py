#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Test real Linux DMA API/SG code with a modeled native backend under sanitizers."""
import argparse, json, os, pathlib, resource, shlex, subprocess
p = argparse.ArgumentParser()
p.add_argument('--output', required=True, type=pathlib.Path)
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
root = pathlib.Path(__file__).resolve().parents[1]
subprocess.run(['python3',str(root/'dma/extract-scatterlist.py')],check=True)
actual = root/'dma/linux_dma_api.c'
copy = a.output/'linux_dma_api.c'
text = actual.read_text()
copy.write_text(text)
command = ['gcc','-std=gnu11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
 '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(root/'include'),
 '-I'+str(root/'upstream'),'-I'+str(root/'dma'),str(copy),str(root/'dma/api_host_test.c'),
 str(root/'build/scatterlist_slice.c'),*shlex.split(os.environ.get('SANITIZER_LDFLAGS','')),'-o',str(a.output/'test')]
for name, cmd in [('build',command),('run',[str(a.output/'test')])]:
 r = subprocess.run(cmd,capture_output=True,text=True,timeout=30)
 (a.output/(name+'.log')).write_text(r.stdout+r.stderr)
 r.check_returncode()
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
controls=[]
for name,before,after in [
 ('lost_map_failure','if (done == nents) return nents;','return nents;'),
 ('lost_rollback','for (int i = 0; i < done;','for (int i = done; i < done;'),
]:
 assert before in text
 copy.write_text(text.replace(before,after))
 subprocess.run(command,check=True,capture_output=True,timeout=30)
 r=subprocess.run([str(a.output/'test')],capture_output=True,text=True,timeout=30)
 controls.append({'name':name,'exit':r.returncode,'stderr':r.stderr})
 assert r.returncode == -6 and 'Assertion' in r.stderr
(a.output/'negative-controls.json').write_text(json.dumps(controls,indent=2)+'\n')
copy.write_text(text)
subprocess.run(command,check=True,capture_output=True,timeout=30)
print('PASS actual Linux DMA API: SG rollback/chaining, invalid directions/attributes, allocation modes; two regressions rejected')
