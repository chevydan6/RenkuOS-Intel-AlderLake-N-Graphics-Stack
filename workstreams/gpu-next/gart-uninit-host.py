# SPDX-License-Identifier: MIT
"""Run production teardown against the real expanding OpenHashTable."""
import pathlib,re,subprocess,tempfile,sys,shutil
source=pathlib.Path(sys.argv[1]);root=pathlib.Path(sys.argv[2])
s=source.read_text();body=re.search(r'^status_t\nagp_uninit\(void\)\n\{.*?^}',s,re.M|re.S).group()
init=re.search(r'^static status_t\nagp_init\(void\)\n\{.*?^}',s,re.M|re.S).group()
prefix=r'''
#include <cassert>
#include <cstdio>
#include <TypeOperation.h>
using BPrivate::TypeOperation;
#include <OpenHashTable.h>
#define TRACE(...) ((void)0)
#define B_PCI_MODULE_NAME "pci"
static int live,refs=0,failID=-1;
static bool failLock,managerLive;
using uint32=unsigned;struct pci_info{};struct module_info{};static module_info* sPCI;static int sDeviceCount,sDeviceInfos[1];
static int get_module(const char*,module_info**){refs++;return B_OK;}
static int get_next_agp_device(uint32*,pci_info&,int&){return -1;}
struct TestLock {bool held=false;};
static TestLock sLock;
static int init_lock(TestLock*,const char*){if(failLock)return -12;assert(!managerLive);managerLive=true;return B_OK;}
[[maybe_unused]] static void uninit_lock(TestLock*){assert(managerLive);managerLive=false;}
struct Autolock {TestLock& lock;Autolock(TestLock& l):lock(l){assert(!l.held);l.held=true;}~Autolock(){lock.held=false;}};
struct Aperture {int id;Aperture* next=nullptr;TestLock mutex;explicit Aperture(int n):id(n){live++;}~Aperture(){assert(!mutex.held);live--;}
 TestLock& Lock(){return mutex;}int DeleteAllMemory(){assert(mutex.held);return id==failID?-5:B_OK;}};
struct Definition {using KeyType=int;using ValueType=Aperture;
 size_t HashKey(const int& id)const{return id;}
 size_t Hash(Aperture* a)const{return a->id;}
 bool Compare(const int& id,Aperture* a)const{return a->id==id;}
 Aperture*& GetLink(Aperture* a)const{return a->next;}};
using ApertureHashTable=BOpenHashTable<Definition>;
static ApertureHashTable sApertureHashTable;
static void put_module(const char*){assert(live==0);refs--;}
'''
suffix=r'''
int main(){
 failLock=true;assert(agp_init()==-12&&refs==0&&!managerLive);failLock=false;
 for(int scenario=0;scenario<2;scenario++) {
  assert(agp_init()==B_OK&&refs==1&&managerLive);failID=scenario?128:-1;
  for(int id=0;id<256;id++)assert(sApertureHashTable.Insert(new Aperture(id))==B_OK);
  assert(sApertureHashTable.TableSize()>8);
  if(scenario){assert(agp_uninit()==-5);assert(refs==1&&live>0&&managerLive);assert(sApertureHashTable.Lookup(128));failID=-1;}
  assert(agp_uninit()==B_OK);assert(live==0&&refs==0&&!managerLive&&sApertureHashTable.TableSize()==0);
  for(int id=0;id<256;id++)assert(!sApertureHashTable.Lookup(id));
 }
 puts("Expanded real hash teardown: 256 objects, success and partial-failure retry passed");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)
 (p/'OS.h').write_text('#pragma once\n#include <cstdint>\n#include <cstdlib>\nusing status_t=int;constexpr int B_OK=0,B_NO_MEMORY=-1,B_ERROR=-2;inline void debugger(const char*){abort();}\n')
 shutil.copy2(root/'headers/private/util/OpenHashTable.h',p/'OpenHashTable.h')
 shutil.copy2(root/'headers/private/shared/TypeOperation.h',p/'TypeOperation.h')
 (p/'test.cpp').write_text(prefix+init+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',d,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
