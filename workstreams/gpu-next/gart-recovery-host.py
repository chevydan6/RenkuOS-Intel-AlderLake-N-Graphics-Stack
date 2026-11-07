#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Trace real GART map/unmap control flow with a faulting singleton backend."""
import argparse
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path, help='production agp_gart.cpp')
parser.add_argument('--emit-cpp', type=pathlib.Path)
parser.add_argument('--guarded-backend', action='store_true', help='model validated Intel singleton guard')
args = parser.parse_args()
source = args.source.read_text()

def function(name):
    start = source.index('\n' + name + '(')
    start = source.rfind('\nstatic ', 0, start) + 1
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        if source[end] == '{':
            depth += 1
        elif source[end] == '}':
            depth -= 1
        end += 1
    return source[start:end]

prefix = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <new>
#include <map>
using uint8=uint8_t;using status_t=int;using aperture_id=int;using addr_t=uintptr_t;
#ifndef B_OK
#define B_OK 0
#endif
#ifndef B_ERROR
#define B_ERROR -1
#endif
#ifndef B_NO_MEMORY
#define B_NO_MEMORY -4
#endif
#ifndef B_BUSY
#define B_BUSY -3
#endif
#ifndef B_ENTRY_NOT_FOUND
#define B_ENTRY_NOT_FOUND -2
#endif
static bool lockError;
struct TestLock {bool held=false;};
struct Autolock {
 TestLock& lock;
 explicit Autolock(TestLock& l):lock(l) {if(l.held)lockError=true;l.held=true;}
 ~Autolock() {lock.held=false;}
};
static TestLock sLock;
static unsigned refs,creates,deletes,generation,backendDeletes;
static bool failMetadata,failInit,failRegistry;
static bool backendMapped;
static const bool guardedBackend=GUARDED_BACKEND;
struct agp_gart_bus_module_info {
 status_t (*create_aperture)(uint8,uint8,uint8,size_t,void**);
 void (*delete_aperture)(void*);
};
using module_info=agp_gart_bus_module_info;
static status_t Create(uint8,uint8,uint8,size_t,void** result) {
 if(!sLock.held)lockError=true;
 if(guardedBackend&&backendMapped)return B_BUSY;
 creates++;generation++;backendMapped=true;*result=NULL;return B_OK;
}
static void Delete(void*) {if(!sLock.held||refs==0)lockError=true;backendDeletes++;backendMapped=false;}
static agp_gart_bus_module_info module={Create,Delete};
static unsigned moduleCursor;
static void* open_module_list(const char*) {moduleCursor=0;return &moduleCursor;}
static status_t read_next_module_name(void*,char* name,size_t*) {
 if(moduleCursor++)return B_ENTRY_NOT_FOUND;
 strcpy(name,"singleton-intel");return B_OK;
}
static status_t get_module(const char*,module_info** result) {refs++;*result=&module;return B_OK;}
static void put_module(const char*) {refs--;}
static void close_module_list(void*) {}
struct Aperture {
 static int nextID;
 static void* operator new(size_t size,const std::nothrow_t&) noexcept {
  return failMetadata?NULL : ::operator new(size,std::nothrow);
 }
 static void operator delete(void* pointer) noexcept {::operator delete(pointer);}
 static void operator delete(void* pointer,const std::nothrow_t&) noexcept {::operator delete(pointer);}
 int id;unsigned createdGeneration;bool failDelete;TestLock lock;
 uint8 bus,device,function;bool pending=false;
 Aperture(agp_gart_bus_module_info*,void*,uint8 b,uint8 d,uint8 f):id(++nextID),createdGeneration(generation),failDelete(false),bus(b),device(d),function(f) {}
 bool NeedsCleanup(uint8 b,uint8 d,uint8 f) const {return pending&&b==bus&&d==device&&f==function;}
 void MarkCleanupPending() {pending=true;}
 ~Aperture() {if(!sLock.held)lockError=true;deletes++;Delete(NULL);refs--;}
 status_t InitCheck() const {return failInit?B_ERROR:B_OK;}
 addr_t Base() const {return 0x800000;}
 int ID() const {return id;}
 TestLock& Lock() {return lock;}
 status_t DeleteAllMemory() {if(!sLock.held||!lock.held)lockError=true;return failDelete?B_ERROR:B_OK;}
};
int Aperture::nextID;
struct Registry {
 std::map<int,Aperture*> entries;
 struct Iterator {
  std::map<int,Aperture*>::iterator current,end;
  bool HasNext() const {return current!=end;}
  Aperture* Next() {return (current++)->second;}
 };
 Iterator GetIterator() {return {entries.begin(),entries.end()};}
 Aperture* Lookup(int id) {auto entry=entries.find(id);return entry==entries.end()?NULL:entry->second;}
 status_t Insert(Aperture* aperture) {if(failRegistry)return B_NO_MEMORY;entries[aperture->ID()]=aperture;return B_OK;}
 bool RemoveUnchecked(Aperture* aperture) {return entries.erase(aperture->ID())!=0;}
 void Remove(Aperture* aperture) {entries.erase(aperture->ID());}
};
using ApertureHashTable=Registry;
static Registry sApertureHashTable;
'''
suffix = r'''
int main() {
 unsigned errors=0,cases=0;
 auto check=[&](bool good,const char* text){cases++;if(!good){fprintf(stderr,"FAIL %s\n",text);errors++;}};
 addr_t base=0;
 int first=map_aperture(0,2,0,0,&base);
 check(first>=0&&base==0x800000&&refs==1&&creates==1&&backendMapped,"initial map owns backend ref");
 check(map_aperture(0,2,0,0,&base)==B_BUSY&&refs==1&&creates==1,
  "active aperture is not reclaimed by another map");
 Aperture* original=sApertureHashTable.Lookup(first);original->failDelete=true;
 check(unmap_aperture(first)==B_ERROR&&sApertureHashTable.Lookup(first)==original
  &&refs==1&&deletes==0&&backendMapped,"failed unmap retains registry and backend ref/mapping");
 original->failDelete=false;
 check(unmap_aperture(first)==B_OK&&!sApertureHashTable.Lookup(first)&&refs==0
  &&deletes==1&&!backendMapped,"later explicit retry of retained ID succeeds");
 first=map_aperture(0,2,0,0,&base);
 original=sApertureHashTable.Lookup(first);original->failDelete=true;
 check(unmap_aperture(first)==B_ERROR,"second failed unmap retains old object");
 unsigned oldGeneration=original->createdGeneration;
 unsigned beforeOther=deletes;
 check(map_aperture(1,2,0,0,&base)==B_BUSY&&deletes==beforeOther
  &&sApertureHashTable.Lookup(first)==original,
  "different PCI bus cannot clean retained aperture");
 check(map_aperture(0,3,0,0,&base)==B_BUSY&&deletes==beforeOther,
  "different PCI device cannot clean retained aperture");
 check(map_aperture(0,2,1,0,&base)==B_BUSY&&deletes==beforeOther,
  "different PCI function cannot clean retained aperture");
 int reopened=map_aperture(0,2,0,0,&base);
 if(guardedBackend) {
  check(reopened==B_ERROR&&sApertureHashTable.entries.size()==1
   &&sApertureHashTable.Lookup(first)==original&&refs==1&&generation==oldGeneration,
   "guarded reopen retains original registry and balances failed module reference");
  original->failDelete=false;
  unsigned beforeDeletes=deletes;
  int finalID=map_aperture(0,2,0,0,&base);
  check(!sApertureHashTable.Lookup(first)&&deletes==beforeDeletes+1,
   "reopen cleans retired aperture without caller retaining its ID");
  check(finalID>=0&&generation==oldGeneration+1&&refs==1,"new creation succeeds after retained cleanup");
  check(unmap_aperture(finalID)==B_OK,"final fake aperture cleanup");
 } else {
  check(reopened>=0&&reopened!=first&&sApertureHashTable.entries.size()==2
   &&sApertureHashTable.Lookup(first)==original&&refs==2&&generation!=oldGeneration,
   "existing manager creates another backend instead of retrying old ID");
  printf("Unsafe reopen reproduced: old generation %u, current %u\n",oldGeneration,generation);
  check(unmap_aperture(reopened)==B_OK,"cleanup reopened fake aperture");
  original->failDelete=false;
  check(unmap_aperture(first)==B_OK,"cleanup retained fake aperture");
 }
 check(refs==0&&sApertureHashTable.entries.empty()&&!lockError,
  "registry references balanced and manager locks held across create/delete");
 unsigned oldCreates=creates,oldDeletes=deletes,oldBackendDeletes=backendDeletes;
 failMetadata=true;
 check(map_aperture(0,2,0,0,&base)==B_NO_MEMORY&&refs==0&&!backendMapped
  &&sApertureHashTable.entries.empty()&&creates==oldCreates+1
  &&deletes==oldDeletes&&backendDeletes==oldBackendDeletes+1,
  "metadata OOM releases backend and module ref without registry insertion");
 failMetadata=false;failInit=true;
 check(map_aperture(0,2,0,0,&base)==B_ERROR&&refs==0&&!backendMapped
  &&sApertureHashTable.entries.empty()&&creates==oldCreates+2
  &&deletes==oldDeletes+1&&backendDeletes==oldBackendDeletes+2,
  "existing metadata init failure destroys aperture and backend exactly once");
 failInit=false;failRegistry=true;base=0x1234;
 unsigned beforeRegistryDelete=backendDeletes;
 check(map_aperture(0,2,0,0,&base)==B_NO_MEMORY&&base==0x1234
  &&refs==0&&!backendMapped&&sApertureHashTable.entries.empty()
  &&backendDeletes==beforeRegistryDelete+1,
  "initial registry publication failure leaves output untouched and releases backend");
 failRegistry=false;
 int afterFault=map_aperture(0,2,0,0,&base);
 check(afterFault>=0&&refs==1&&backendMapped,"creation succeeds after metadata failures");
 check(unmap_aperture(afterFault)==B_OK&&refs==0&&!backendMapped&&!lockError,
  "post-fault cleanup balances module references and locks");
 if(!errors)printf("%u GART retention, retry, reopen and metadata/module cleanup cases passed\n",cases);
 return errors?1:0;
}
'''
prefix = prefix.replace('GUARDED_BACKEND', 'true' if args.guarded_backend else 'false')
fixture = prefix + function('map_aperture') + '\n' + function('unmap_aperture') + suffix
if args.emit_cpp:
    args.emit_cpp.write_text(fixture)
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='gpu-gart-retention-') as directory:
    tmp = pathlib.Path(directory)
    (tmp/'test.cpp').write_text(fixture)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',
                    '-fno-exceptions','-fno-rtti','-fsanitize=address,undefined',
                    str(tmp/'test.cpp'),'-o',str(tmp/'test')],check=True)
    raise SystemExit(subprocess.run([str(tmp/'test')]).returncode)
