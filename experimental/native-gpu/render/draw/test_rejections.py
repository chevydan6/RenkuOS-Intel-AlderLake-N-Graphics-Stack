#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Prove the XML checker rejects substantive packed-pipeline regressions."""
from pathlib import Path
import contextlib,io,re,runpy,subprocess,sys,tempfile
here=Path(__file__).resolve().parent
if len(sys.argv)!=2: raise SystemExit('usage: test_rejections.py MESA_SOURCE')
mesa=sys.argv[1];sys.argv=[str(here/'check_image.py'),mesa]
with contextlib.redirect_stdout(io.StringIO()): decoded=runpy.run_path(str(here/'check_image.py'))
original=(here/'RcsDrawGenerated.h').read_text();batch=decoded['arrays']['Batch']
for name,field,value in [('3DSTATE_VS','Enable',0),('3DSTATE_PS_EXTRA','Pixel Shader Valid',0),
 ('3DSTATE_VF_TOPOLOGY','Primitive Topology Type',15),('3DSTATE_PS','Kernel Start Pointer 2',0x5040)]:
 words=decoded['cmds'][name][0]
 offset=next(i for i in range(len(batch)) if batch[i:i+len(words)]==words)
 f=next(f for f in decoded['fields'](name) if f.attrib['name']==field)
 start,width,lo=decoded['bounds'](f);index=offset+start//32;bit=start%32
 mutated=batch.copy();mask=((1<<width)-1)<<bit
 mutated[index]=(mutated[index]&~mask)|((value>>lo)<<bit)
 body='\n'+','.join(f'0x{w:08x}U' for w in mutated)+'\n'
 changed=re.sub(r'(uint32_t Batch\[[^]]*\] = \{).*?(\};)',lambda m:m[1]+body+m[2],original,count=1,flags=re.S)
 with tempfile.TemporaryDirectory(prefix='draw-negative-') as tmp:
  path=Path(tmp)/'RcsDrawGenerated.h';path.write_text(changed)
  result=subprocess.run([sys.executable,str(here/'check_image.py'),mesa,'--header',str(path)],capture_output=True,text=True)
  assert result.returncode!=0 and 'AssertionError' in result.stderr,(name,result.stdout,result.stderr)
  print('Rejected corrupt packed image:',name,field)
