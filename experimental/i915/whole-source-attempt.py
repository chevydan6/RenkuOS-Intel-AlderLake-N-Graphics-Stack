#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile a complete byte-exact i915 translation unit, retaining failures."""
import argparse, hashlib, json, pathlib, subprocess
p=argparse.ArgumentParser()
p.add_argument('--linux-source',required=True,type=pathlib.Path)
p.add_argument('--unit',required=True)
p.add_argument('--log',required=True,type=pathlib.Path)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent
manifest=json.loads((root/'upstream-manifest.json').read_text())
if a.unit not in [f['path'] for f in manifest['files']]:p.error('unit must be an unchanged manifested import')
revision=subprocess.check_output(['git','-C',str(a.linux_source),'rev-parse','HEAD'],text=True).strip()
if revision!=manifest['linuxRevision']:p.error('Linux revision mismatch')
subprocess.run(['python3',str(root/'verify-upstream.py')],check=True)
build=json.loads((root/'build/target/manifest.json').read_text())
command=build['objects'][0]['command'];command=command[:command.index('-MMD')]
for directory in ['drivers/gpu/drm/i915','drivers/gpu/drm/i915/gem','drivers/gpu/drm/i915/gt','include','include/uapi']:
 command+=['-I'+str(a.linux_source/directory)]
output=root/'build'/('complete-'+a.unit.replace('/','-')+'.o')
output.unlink(missing_ok=True)
source=root/'upstream'/a.unit
command+=['-fmax-errors=12','-c',str(source),'-o',str(output)]
with a.log.open('x') as log:
 log.write('Linux source: '+revision+'\nsource_sha256: '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+' '.join(command)+'\n');log.flush()
 r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
 log.write('\ncompiler exit: %d\n'%r.returncode)
print(a.unit,'compiler exit:',r.returncode)
raise SystemExit(r.returncode)
