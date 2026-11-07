# SPDX-License-Identifier: MIT
import pathlib,subprocess,tempfile,json,resource
resource.setrlimit(resource.RLIMIT_CORE,(0,0))
p=pathlib.Path('/home/dan/renkuos-development/gpu-driver')
h=pathlib.Path('/home/dan/renkuos-development/source/gpu-next-worktree/headers/private/graphics/intel_extreme/IntelApertureLeaseState.h').read_text()
s=(p/'tests/intel-aperture-lease-test.cpp').read_text().split('int\nmain()')[0]
cases=[('control',h,0,0),('missing_pin_guard',h.replace('fLeases[i].live || fLeases[i].operations != 0','fLeases[i].live'),0,-6),('forgotten_failure',h.replace('fUnresolvedBacking = true;','fUnresolvedBacking = false;'),1,-6)]
results=[]
with tempfile.TemporaryDirectory(prefix='renku-owner-controls-') as d:
 d=pathlib.Path(d)
 for name,header,mask,expected in cases:
  (d/'IntelApertureLeaseState.h').write_text(header)
  (d/'test.cpp').write_text(s+'int main() { CheckRetiringOperations('+str(mask)+', 0); }\n')
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-pthread','-I',str(d),str(d/'test.cpp'),'-o',str(d/'test')],check=True,capture_output=True)
  r=subprocess.run([str(d/'test')],capture_output=True,text=True,timeout=15)
  results.append(dict(case=name,exit=r.returncode,expected=expected,stderr=r.stderr,passed=r.returncode==expected))
print(json.dumps(results,indent=2))
assert all(r['passed'] for r in results)
