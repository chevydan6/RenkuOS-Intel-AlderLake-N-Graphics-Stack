#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise actual accelerant size_t-to-uint32 allocation boundary."""
import argparse
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path, help='production memory.cpp')
parser.add_argument('--emit-cpp', type=pathlib.Path, help='emit standalone fixture only')
args = parser.parse_args()
source = args.source.read_text()
start = source.index('status_t\nintel_allocate_memory(')
body = source[start:]
prefix = r'''
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
using uint32=uint32_t;using addr_t=uintptr_t;using status_t=int;
#ifndef B_OK
#define B_OK 0
#endif
#ifndef B_BAD_VALUE
#define B_BAD_VALUE -1
#endif
enum {INTEL_PRIVATE_DATA_MAGIC=0x12345678,INTEL_ALLOCATE_GRAPHICS_MEMORY=10};
struct intel_allocate_graphics_memory {uint32 magic,size,alignment,flags;addr_t buffer_base;};
struct Info {int device;};static Info info={7};static Info* gInfo=&info;
static unsigned calls;static uint32 lastSize,lastFlags;static bool fail,bad;
static int ioctl(int fd,unsigned opcode,void* pointer,size_t size) {
 calls++;if(fd!=7||opcode!=INTEL_ALLOCATE_GRAPHICS_MEMORY||size!=sizeof(intel_allocate_graphics_memory))bad=true;
 auto& request=*(intel_allocate_graphics_memory*)pointer;
 if(request.magic!=INTEL_PRIVATE_DATA_MAGIC||request.alignment!=0)bad=true;
 lastSize=request.size;lastFlags=request.flags;
 if(fail){errno=EIO;return -1;}
 request.buffer_base=0xffffa0001000ULL;return 0;
}
'''
suffix = r'''
int main() {
 unsigned errors=0,cases=0;
 auto check=[&](bool condition,const char* message){cases++;if(!condition){fprintf(stderr,"FAIL %s\n",message);errors++;}};
 const uint64_t values[]={0,1,4095,4096,8192,UINT32_MAX-1ULL,UINT32_MAX,
  UINT32_MAX+1ULL,UINT32_MAX+4096ULL,UINT64_MAX};
 for(uint64_t value:values) {
  if(value>SIZE_MAX)continue;
  addr_t base=99;calls=0;bad=false;fail=false;
  status_t result=intel_allocate_memory((size_t)value,3,base);
  bool valid=value>0&&value<=UINT32_MAX;
  check(valid ? result==B_OK&&calls==1&&lastSize==value&&lastFlags==3
   &&base==0xffffa0001000ULL&&!bad : result==B_BAD_VALUE&&calls==0&&base==99,
   "32-bit ABI boundary preserves exact requested size or rejects before ioctl");
 }
 for(unsigned i=0;i<100;i++) {
  uint64_t value=(uint64_t(i+1)<<32)|((i*7919)&0xffffffffU);
  if(value>SIZE_MAX)continue;
  addr_t base=99;calls=0;
  check(intel_allocate_memory((size_t)value,0,base)==B_BAD_VALUE&&calls==0&&base==99,
   "large request cannot alias low 32-bit size");
 }
 calls=0;fail=true;addr_t base=99;
 check(intel_allocate_memory(8192,1,base)==EIO&&calls==1&&base==99,
  "allocation backend failure preserves caller base");
 if(errors)return 1;
 printf("%u actual accelerant allocation size-boundary cases passed\n",cases);
}
'''
fixture = prefix + body + suffix
if args.emit_cpp:
    args.emit_cpp.write_text(fixture)
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='gpu-memory-size-') as directory:
    tmp = pathlib.Path(directory)
    (tmp/'test.cpp').write_text(fixture)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',
                    '-fno-exceptions','-fno-rtti','-fsanitize=address,undefined',
                    str(tmp/'test.cpp'),'-o',str(tmp/'test')],check=True)
    raise SystemExit(subprocess.run([str(tmp/'test')]).returncode)
