# SPDX-License-Identifier: MIT
"""Check production GART placement against reserved-prefix and alignment bounds."""
import pathlib,re,subprocess,tempfile,sys
s=pathlib.Path(sys.argv[1]).read_text();body=re.search(r'^status_t\nAperture::_Insert\([^)]*\)\n\{.*?^}',s,re.M|re.S).group()
prefix=r'''
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cassert>
using addr_t=uintptr_t;using uint32=uint32_t;using status_t=int;
constexpr int B_OK=0,B_BAD_VALUE=-1,B_NO_MEMORY=-2;
constexpr size_t B_PAGE_SIZE=4096;
constexpr uint32 B_APERTURE_NON_RESERVED=1,B_APERTURE_NEED_PHYSICAL=2;
#define ROUNDUP(x,a) ((((x)+(a)-1)/(a))*(a))
struct aperture_memory {addr_t base;size_t size;aperture_memory* next;};
struct aperture_info {addr_t base;size_t size,reserved_size;};
struct Aperture {aperture_info fInfo;aperture_memory* fFirstMemory;
 int _Insert(aperture_memory*,size_t,size_t,uint32);};
'''
suffix=r'''
int main(){unsigned cases=0;
 for(size_t reserved=0;reserved<=16;reserved++) {
  for(size_t pages=1;pages<=16;pages++) {
   for(size_t alignment:{size_t(4096),size_t(8192),size_t(16384)}) {
    Aperture a{{0x100000,16*4096,reserved*4096},nullptr};
    aperture_memory memory{73,91,nullptr};
    addr_t start=ROUNDUP(a.fInfo.base+reserved*4096,alignment);
    bool fits=start<a.fInfo.base+a.fInfo.size&&pages*4096<=a.fInfo.base+a.fInfo.size-start;
    int status=a._Insert(&memory,pages*4096,alignment,B_APERTURE_NON_RESERVED);
    assert((status==B_OK)==fits);
    if(fits)assert(memory.base==start&&memory.size==pages*4096&&a.fFirstMemory==&memory);
    else assert(memory.base==73&&memory.size==91&&a.fFirstMemory==nullptr);
    cases++;
   }
  }
 }
 printf("%u production empty-aperture reserved/alignment placement cases passed\n",cases);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text('#include <initializer_list>\n'+prefix+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
