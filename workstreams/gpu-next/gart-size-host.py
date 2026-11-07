# SPDX-License-Identifier: MIT
"""Test production GART allocate callback size checks before lookup."""
import pathlib,re,subprocess,tempfile,sys
s=pathlib.Path(sys.argv[1]).read_text()
body=re.search(r'^static status_t\nallocate_memory\([^)]*\)\n\{.*?^}',s,re.M|re.S).group()
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <initializer_list>
using status_t=int;using aperture_id=int;using addr_t=uintptr_t;using phys_addr_t=uint64_t;using uint32=uint32_t;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_ENTRY_NOT_FOUND=-2,B_NO_MEMORY=-3,B_BUSY=-16;
constexpr size_t B_PAGE_SIZE=4096;
constexpr uint32 APERTURE_PUBLIC_FLAGS_MASK=1,B_APERTURE_NEED_PHYSICAL=1;
#define ROUNDUP(x,a) (((x)+(a)-1)&~((a)-1))
#define ERROR(...) ((void)0)
struct Page {uint64_t physical_page_number;};
struct aperture_memory {Page* page;addr_t base;};
static unsigned lookups,creates;
static size_t rounded;
static bool available=true;
static bool pending=false;
struct TestLock{};static TestLock sLock;
struct Autolock {explicit Autolock(TestLock&) {}};
struct Aperture {
 bool IsCleanupPending() const {return pending;}
 TestLock lock;TestLock& Lock(){return lock;}
 aperture_memory* CreateMemory(size_t size,size_t,uint32){creates++;rounded=size;return nullptr;}
 int AllocateMemory(aperture_memory*,uint32){return 0;}
 int BindMemory(aperture_memory*,int,int){return 0;}
 int DeleteMemory(aperture_memory*){return 0;}
};
static Aperture aperture;
static Aperture* get_aperture(int){lookups++;return available?&aperture:nullptr;}
'''
suffix=r'''
int main(){
 addr_t base=73;phys_addr_t physical=91;unsigned cases=0;
 for(size_t value: {size_t(0),SIZE_MAX,SIZE_MAX-1,SIZE_MAX-4094}) {
  lookups=creates=0;
  assert(allocate_memory(1,value,0,0,&base,&physical)==B_BAD_VALUE);
  assert(!lookups&&!creates&&base==73&&physical==91);cases++;
 }
 for(size_t value: {size_t(1),size_t(4095),size_t(4096),size_t(4097),SIZE_MAX-4095}) {
  lookups=creates=0;
  assert(allocate_memory(1,value,0,0,&base,&physical)==B_NO_MEMORY);
  assert(lookups==1&&creates==1&&rounded==ROUNDUP(value,B_PAGE_SIZE));
  assert(base==73&&physical==91);cases++;
 }
 lookups=creates=0;
 assert(allocate_memory(1,1,0,2,&base,&physical)==B_BAD_VALUE&&!lookups);
 assert(allocate_memory(1,1,0,0,nullptr,&physical)==B_BAD_VALUE&&!lookups);
 pending=true;lookups=creates=0;
 assert(allocate_memory(1,4096,0,0,&base,&physical)==B_BUSY);
 assert(lookups==1&&!creates&&base==73&&physical==91);
 pending=false;
 available=false;
 assert(allocate_memory(1,4096,0,0,&base,&physical)==B_ENTRY_NOT_FOUND&&!creates);
 printf("%u production GART size-boundary cases plus flags/null/missing-ID checks passed\n",cases);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
