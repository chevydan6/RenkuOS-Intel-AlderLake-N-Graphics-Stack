#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Actual OpenHashTable bucket-allocation failure + production memory publication."""
from pathlib import Path
import argparse,re,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('source',type=Path);args=p.parse_args()
root=Path(__file__).parent
s=args.source.read_text()
functions=[re.search(r'^'+kind+r'\nAperture::'+name+r'\([^)]*\)\n\{.*?^}',s,re.M|re.S).group() for kind,name in [('aperture_memory \\*','CreateMemory'),('void','_Remove')]]
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include "OpenHashTable.h"
using addr_t=uintptr_t;using uint32=uint32_t;
#define ERROR(...) ((void)0)
#define TRACE(...) ((void)0)
static bool failBucket;static unsigned bucketAttempts;static int live;
struct aperture_memory {addr_t base;size_t size;uint32 flags;size_t bound_size;void* pages;aperture_memory *next,*hash_link;aperture_memory(){live++;}~aperture_memory(){live--;}};
struct Definition {using KeyType=addr_t;using ValueType=aperture_memory;size_t HashKey(addr_t k)const{return k>>12;}size_t Hash(ValueType* v)const{return HashKey(v->base);}bool Compare(addr_t k,ValueType* v)const{return k==v->base;}ValueType*& GetLink(ValueType* v)const{return v->hash_link;}};
struct FaultAllocator {void* Allocate(size_t n)const{bucketAttempts++;return failBucket?nullptr:malloc(n);}void Free(void* p)const{free(p);}};
using Hash=BOpenHashTable<Definition,true,false,FaultAllocator>;
struct Aperture {Hash fHashTable;aperture_memory* fFirstMemory=nullptr;
int _Insert(aperture_memory* m,size_t size,size_t,uint32){m->base=0x100000;m->size=size;m->next=fFirstMemory;fFirstMemory=m;return B_OK;}
void _Remove(aperture_memory*);aperture_memory* CreateMemory(size_t,size_t,uint32);};
'''
suffix=r'''
int main(){
 Aperture a;aperture_memory existing;existing.next=nullptr;a.fFirstMemory=&existing;
 for(unsigned n=0;n<100;n++) {failBucket=true;assert(a.CreateMemory(4096,0,7)==nullptr);assert(live==1&&a.fFirstMemory==&existing&&existing.next==nullptr);assert(a.fHashTable.TableSize()==0&&a.fHashTable.Lookup(0x100000)==nullptr);}
 assert(bucketAttempts==100);failBucket=false;auto* m=a.CreateMemory(4096,0,7);
 assert(m&&live==2&&a.fHashTable.Lookup(0x100000)==m&&m->next==&existing);
 assert(a.fHashTable.TableSize()==8&&bucketAttempts==101);
 assert(m->flags==7&&m->bound_size==0&&m->pages==nullptr);
 a._Remove(m);assert(a.fHashTable.Remove(m));delete m;
 assert(live==1&&a.fFirstMemory==&existing&&a.fHashTable.IsEmpty());
 puts("PASS: 100 actual initial-bucket allocation failures, preserved list/ownership, successful retry and cleanup");
}
'''
with tempfile.TemporaryDirectory() as tmp:
 d=Path(tmp)
 (d/'OpenHashTable.h').write_bytes((root/'evidence/renku__headers__private__util__OpenHashTable.h').read_bytes())
 (d/'OS.h').write_text('#pragma once\n#include <cstdint>\nusing status_t=int;constexpr int B_OK=0,B_NO_MEMORY=-1;inline void debugger(const char*){}\n')
 (d/'TypeOperation.h').write_text('#pragma once\ntemplate<typename T>struct TypeOperation{using ConstRefT=const T&;};\n')
 (d/'test.cpp').write_text(prefix+'\n'.join(functions)+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',tmp,str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,timeout=5)
