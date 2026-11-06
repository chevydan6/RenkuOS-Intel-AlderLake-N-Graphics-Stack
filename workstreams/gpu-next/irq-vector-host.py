#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exhaustively check production vector allocation with modeled kernel storage.
No interrupt dispatch, PCI accesses, native kernel or physical hardware involved.
"""
import argparse
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
args = parser.parse_args()
source = args.source.read_text()
match = re.search(r'^status_t\nallocate_io_interrupt_vectors\([^)]*\)\n\{.*?^}', source, re.M | re.S)
if not match:
    raise SystemExit('missing production allocator')
prefix = r'''
#include <cstdint>
#include <cstdio>
#include <cstring>
using int32=int32_t;using uint16=uint16_t;using status_t=int;
constexpr int NUM_IO_VECTORS=8;
constexpr int B_OK=0,B_NO_MEMORY=-1,B_BAD_VALUE=-2;
enum interrupt_type {INTERRUPT_TYPE_UNKNOWN,INTERRUPT_TYPE_IRQ};
struct Assignment {int irq,count;};
struct Vector {interrupt_type type;Assignment* assigned_cpu;};
static Vector sVectors[NUM_IO_VECTORS];
static Assignment sVectorCPUAssignments[NUM_IO_VECTORS];
static bool sAllocatedIOInterruptVectors[NUM_IO_VECTORS];
static int sIOInterruptVectorAllocationLock;
struct MutexLocker {explicit MutexLocker(int*){}};
#define dprintf(...) ((void)0)
'''
tests = r'''
int main()
{
 unsigned failures=0,cases=0;
 // Eight slots enumerate all occupancy maps and all supported request sizes.
 for(unsigned occupied=0;occupied<(1u<<NUM_IO_VECTORS);occupied++){
  for(int count=1;count<=NUM_IO_VECTORS;count++){
   for(int slot=0;slot<NUM_IO_VECTORS;slot++){
    sAllocatedIOInterruptVectors[slot]=(occupied&(1u<<slot))!=0;
    sVectors[slot]={INTERRUPT_TYPE_UNKNOWN,nullptr};
    sVectorCPUAssignments[slot]={-9,-9};
   }
   int expected=-1;
   for(int start=0;start<=NUM_IO_VECTORS-count;start++){
    unsigned range=((1u<<count)-1)<<start;
    if((occupied&range)==0){expected=start;break;}
   }
   int first=-1234;
   int result=allocate_io_interrupt_vectors(count,&first,INTERRUPT_TYPE_IRQ);
   bool ok=expected<0?(result==B_NO_MEMORY&&first==-1234)
    :(result==B_OK&&first==expected);
   for(int slot=0;slot<NUM_IO_VECTORS;slot++){
    bool added=expected>=0&&slot>=expected&&slot<expected+count;
    bool wasUsed=(occupied&(1u<<slot))!=0;
    ok=ok&&(sAllocatedIOInterruptVectors[slot]==(wasUsed||added));
    ok=ok&&(sVectors[slot].type==(added?INTERRUPT_TYPE_IRQ:INTERRUPT_TYPE_UNKNOWN));
    ok=ok&&(sVectors[slot].assigned_cpu==(added?&sVectorCPUAssignments[expected]:nullptr));
    ok=ok&&(sVectorCPUAssignments[slot].irq==(slot==expected?expected:-9));
    ok=ok&&(sVectorCPUAssignments[slot].count==(slot==expected?count:-9));
   }
   if(!ok){
    if(failures<8||(occupied==255&&count==1))printf("FAIL occupancy=0x%x count=%d result=%d first=%d expected=%d\n",occupied,count,result,first,expected);
    failures++;
   }
   cases++;
  }
 }
 printf("%u allocator cases; %u failures (host model, actual allocator function).\n",cases,failures);
 return failures?2:0;
}
'''
with tempfile.TemporaryDirectory(prefix='renku-vector-') as directory:
    cpp=pathlib.Path(directory)/'test.cpp'
    binary=pathlib.Path(directory)/'test'
    cpp.write_text(prefix+match.group()+tests)
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True)
    result=subprocess.run([str(binary)],timeout=15)
    raise SystemExit(result.returncode)
