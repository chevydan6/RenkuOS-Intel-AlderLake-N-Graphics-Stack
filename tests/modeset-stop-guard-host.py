#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import pathlib,tempfile,subprocess,sys
s=pathlib.Path(sys.argv[1]).read_text();a=s.index('\tstatus_t powerStatus = set_display_power_mode(B_DPMS_OFF);');b=s.index('\n\t// free old and allocate',a)
prefix=r'''
#include <cstdio>
using status_t=int;
enum {B_OK=0,B_ERROR=-1,B_DPMS_OFF=3};
struct Shared{unsigned dpms_mode;} sharedInfo;
unsigned calls=0,frees=0,lastMode=0;status_t offResult=0;
status_t set_display_power_mode(unsigned mode){calls++;lastMode=mode;return calls==1?offResult:B_OK;}
status_t Exercise(){
'''
suffix=r'''
 frees++;return B_OK;}
int main(){unsigned errors=0;
for(unsigned scenario=0;scenario<3;scenario++){
 calls=frees=0;sharedInfo.dpms_mode=scenario==2?3:0;offResult=scenario?B_ERROR:B_OK;
 status_t result=Exercise();
 if(!scenario){if(result!=B_OK||frees!=1||calls!=1)errors++;}
 else if(result!=B_ERROR||frees||calls!=2||lastMode!=sharedInfo.dpms_mode)errors++;
}
if(errors){std::fprintf(stderr,"%u modeset stop guards failed\n",errors);return 1;}
std::puts("3 actual modeset OFF failure guards retain framebuffer backing");}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)/'t.cpp';p.write_text(prefix+s[a:b]+suffix);binary=pathlib.Path(d)/'t'
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
