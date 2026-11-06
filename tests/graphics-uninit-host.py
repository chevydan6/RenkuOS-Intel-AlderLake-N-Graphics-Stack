#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import pathlib,tempfile,subprocess,sys
s=pathlib.Path(sys.argv[1]).read_text();a=s.index('void\nintel_uninit_accelerant(void)');b=s.index('\n\nstatus_t\nintel_get_accelerant_device_info',a)
prefix=r'''
#include <cstdio>
#include <cstddef>
using uint32=unsigned;
enum{B_OK=0,B_ERROR=-1};
#define CALLED()
#define ERROR(...)
bool allow=true,fail=false;unsigned held=0,cleanups=0,areas=0,locks=0,common=0;bool orderError=false;
struct Port{int ShutdownGraphics(){cleanups++;if(!held)orderError=true;return fail?B_ERROR:B_OK;}} port;
struct intel_shared_info{int accelerant_lock=0,engine_lock=0,primary_ring_buffer=0;} shared;
struct Info{intel_shared_info* shared_info;unsigned port_count;Port* ports[1];bool is_clone;int mode_list_area;void* mode_list;} info,*gInfo=&info;
class Autolock{bool ok;public:Autolock(int&):ok(allow){if(ok)held++;}~Autolock(){if(ok)held--;}bool IsLocked(){return ok;}};
void delete_area(int){areas++;if(held)orderError=true;}
void uninit_lock(int*){locks++;if(held)orderError=true;}
void uninit_ring_buffer(int&){}
void uninit_common(){common++;if(held)orderError=true;}
'''
suffix=r'''
int main(){unsigned errors=0;
for(unsigned scenario=0;scenario<4;scenario++){
 held=cleanups=areas=locks=common=0;orderError=false;allow=scenario!=3;fail=scenario==2;
 info={&shared,1,{&port},scenario==1,3,(void*)1};
 intel_uninit_accelerant();
 if(scenario<2){if(cleanups!=1||areas!=1||common!=1||locks!=(scenario==1?0U:2U)||info.mode_list!=NULL)errors++;}
 else if(areas||common||locks||info.mode_list!=(void*)1)errors++;
 if(orderError||held)errors++;
}
if(errors){std::fprintf(stderr,"%u uninit ordering failures\n",errors);return 1;}
std::puts("4 actual graphics shutdown ordering, clone and retention cases passed");}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)/'t.cpp';p.write_text(prefix+s[a:b]+suffix);b=pathlib.Path(d)/'t'
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p),'-o',str(b)],check=True)
 subprocess.run([str(b)],check=True)
