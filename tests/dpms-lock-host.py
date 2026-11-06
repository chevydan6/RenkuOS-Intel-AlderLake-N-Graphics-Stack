#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import pathlib,sys,tempfile,subprocess
s=pathlib.Path(sys.argv[1]).read_text();s=s[s.index('status_t\nintel_set_dpms_mode('):]
prefix='''
#include <cstdio>
using uint32=unsigned;using status_t=int;
enum { B_OK=0,B_ERROR=-1,B_BAD_VALUE=-2,B_DPMS_ON=0,B_DPMS_STAND_BY=1,B_DPMS_SUSPEND=2,B_DPMS_OFF=3 };
#define CALLED()
bool lockAllowed=true;unsigned locks=0,held=0,calls=0;bool unlockedCall=false;
struct Shared {int accelerant_lock;uint32 dpms_mode;} shared;
struct Info {Shared* shared_info;} info={&shared},*gInfo=&info;
class Autolock {bool ok;public:Autolock(int&):ok(lockAllowed){locks++;if(ok)held++;}~Autolock(){if(ok)held--;}bool IsLocked(){return ok;}};
void set_display_power_mode(uint32){calls++;if(!held)unlockedCall=true;}
'''
suffix='''
int main(){unsigned errors=0;
 shared.dpms_mode=77;lockAllowed=false;if(intel_set_dpms_mode(B_DPMS_ON)!=B_ERROR||calls||shared.dpms_mode!=77||held)errors++;
 lockAllowed=true;if(intel_set_dpms_mode(999)!=B_BAD_VALUE||calls||shared.dpms_mode!=77||held)errors++;
 for(unsigned mode=0;mode<=3;mode++)if(intel_set_dpms_mode(mode)!=B_OK||shared.dpms_mode!=mode||held)errors++;
 if(unlockedCall||calls!=4||locks!=5)errors++;
 if(errors){std::fprintf(stderr,"%u DPMS lock failures\\n",errors);return 1;}std::puts("6 public DPMS lock and validation cases passed");}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)/'t.cpp';p.write_text(prefix+s+suffix);b=pathlib.Path(d)/'t'
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(p),'-o',str(b)],check=True)
 subprocess.run([str(b)],check=True)
