#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fault injection against actual production allocation/free ioctl case bodies."""
import argparse
import pathlib
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path, help='production device.cpp')
parser.add_argument('--emit-cpp', type=pathlib.Path, help='write standalone fixture without compiling or running')
args = parser.parse_args()
source = args.source.read_text()
cases = []
for opcode in ('INTEL_ALLOCATE_GRAPHICS_MEMORY', 'INTEL_FREE_GRAPHICS_MEMORY'):
    start = source.index('case ' + opcode + ':')
    end = source.index('\n\t\tcase ', start + 1)
    cases.append(source[start:end])
prefix = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
using uint32=uint32_t;using addr_t=uintptr_t;using status_t=int;
enum {B_OK=0,B_BAD_VALUE=-1,B_BAD_ADDRESS=-2,B_ERROR=-3,B_NO_MEMORY=-4,B_DEV_INVALID_IOCTL=-5,
 INTEL_ALLOCATE_GRAPHICS_MEMORY=10,INTEL_FREE_GRAPHICS_MEMORY=11,
 INTEL_PRIVATE_DATA_MAGIC=0x12345678};
#define ERROR(...) ((void)0)
struct intel_allocate_graphics_memory {uint32 magic,size,alignment,flags;addr_t buffer_base;};
struct intel_free_graphics_memory {uint32 magic;addr_t buffer_base;};
struct intel_info {};
static intel_info infoObject;
static unsigned copies,failCopy,allocations,frees,live;
static status_t allocateResult,freeResult;
static bool badArguments;
static const addr_t allocatedBase=0xffffa0001000ULL;
static status_t user_memcpy(void* dst,const void* src,size_t size) {
 copies++;if(copies==failCopy||!dst||!src)return B_BAD_ADDRESS;
 memcpy(dst,src,size);return B_OK;
}
static status_t intel_allocate_memory(intel_info&,size_t size,size_t alignment,uint32 flags,addr_t* base) {
 allocations++;if(size!=8192||alignment!=4096||flags!=1)badArguments=true;
 if(allocateResult!=B_OK)return allocateResult;
 *base=allocatedBase;live++;return B_OK;
}
static status_t intel_free_memory(intel_info&,addr_t base) {
 frees++;if(base!=allocatedBase||live!=1)badArguments=true;
 if(freeResult!=B_OK)return freeResult;
 live--;return B_OK;
}
static void Reset() {
 copies=failCopy=allocations=frees=live=0;allocateResult=freeResult=B_OK;badArguments=false;
}
static status_t Run(uint32 opcode,void* buffer,size_t bufferLength) {
 (void)bufferLength;
 intel_info* info=&infoObject;
 switch(opcode) {
'''
suffix = r'''
 default:break;
 }
 return B_DEV_INVALID_IOCTL;
}
int main() {
 unsigned errors=0,cases=0;
 auto check=[&](bool good,const char* name){cases++;if(!good){fprintf(stderr,"FAIL %s\n",name);errors++;}};
 intel_allocate_graphics_memory allocation={INTEL_PRIVATE_DATA_MAGIC,8192,4096,1,99};
 intel_free_graphics_memory release={INTEL_PRIVATE_DATA_MAGIC,allocatedBase};
 Reset();check(Run(10,&allocation,sizeof(allocation))==B_OK&&allocations==1&&live==1
  &&frees==0&&copies==2&&allocation.buffer_base==allocatedBase&&!badArguments,"allocate succeeds and transfers result");
 check(Run(11,&release,sizeof(release))==B_OK&&live==0&&frees==1&&!badArguments,"delivered allocation remains freeable");
 for(size_t size=0;size<sizeof(allocation);size++) {
  Reset();check(Run(10,&allocation,size)==B_BAD_VALUE&&copies==0&&allocations==0&&frees==0,"short allocate rejected before usercopy");
 }
 for(size_t size=0;size<sizeof(release);size++) {
  Reset();check(Run(11,&release,size)==B_BAD_VALUE&&copies==0&&frees==0,"short free rejected before usercopy");
 }
 Reset();failCopy=1;check(Run(10,&allocation,sizeof(allocation))==B_BAD_ADDRESS&&allocations==0&&frees==0,"allocate copyin fault");
 Reset();check(Run(10,NULL,sizeof(allocation))==B_BAD_ADDRESS&&allocations==0,"null allocate request");
 Reset();allocation.magic=0;check(Run(10,&allocation,sizeof(allocation))==B_BAD_VALUE&&allocations==0&&copies==1,"allocate magic guard");allocation.magic=INTEL_PRIVATE_DATA_MAGIC;
 Reset();allocateResult=B_NO_MEMORY;check(Run(10,&allocation,sizeof(allocation))==B_NO_MEMORY&&copies==1&&allocations==1&&live==0&&frees==0,"allocator failure has no teardown or copyout");
 Reset();failCopy=2;check(Run(10,&allocation,sizeof(allocation))==B_BAD_ADDRESS&&copies==2&&allocations==1&&frees==1&&live==0&&!badArguments,"copyout fault rolls back exact new allocation");
 Reset();failCopy=2;freeResult=B_ERROR;check(Run(10,&allocation,sizeof(allocation))==B_ERROR&&allocations==1&&frees==1&&live==1&&!badArguments,"failed rollback retains tracked backing");
 Reset();live=1;failCopy=1;check(Run(11,&release,sizeof(release))==B_BAD_ADDRESS&&frees==0&&live==1,"free copyin fault preserves backing");
 Reset();release.magic=0;check(Run(11,&release,sizeof(release))==B_DEV_INVALID_IOCTL&&frees==0,"free magic guard");release.magic=INTEL_PRIVATE_DATA_MAGIC;
 Reset();live=1;freeResult=B_ERROR;check(Run(11,&release,sizeof(release))==B_ERROR&&frees==1&&live==1,"free failure propagated and backing retained");
 Reset();check(Run(10,&allocation,sizeof(allocation)+16)==B_OK&&live==1&&!badArguments,"larger caller buffer remains compatible");
 Reset();
 for(unsigned i=0;i<64;i++) {
  failCopy=copies+2;
  check(Run(10,&allocation,sizeof(allocation))==B_BAD_ADDRESS&&live==0
   &&allocations==i+1&&frees==i+1&&!badArguments,"repeated failed delivery does not exhaust allocation pool");
 }
 if(errors)return 1;
 printf("%u actual allocation/free ioctl bounds, ownership-transfer and rollback cases passed\n",cases);
}
'''
fixture = prefix + '\n'.join(cases) + suffix
if args.emit_cpp is not None:
    args.emit_cpp.write_text(fixture)
    print('Wrote standalone production-case fixture: ' + str(args.emit_cpp))
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='gpu-memory-ioctl-') as directory:
    tmp = pathlib.Path(directory)
    path = tmp/'test.cpp'
    path.write_text(fixture)
    binary = tmp/'test'
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(path),'-o',str(binary)],check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
