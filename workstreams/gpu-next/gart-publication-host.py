# SPDX-License-Identifier: MIT
"""Fault-inject metadata hash publication in actual CreateMemory and _Remove."""
import pathlib,re,subprocess,tempfile,sys
s=pathlib.Path(sys.argv[1]).read_text()
functions=[]
for result,name in [('aperture_memory \\*','CreateMemory'),('void','_Remove')]:
 functions.append(re.search(r'^'+result+r'\nAperture::'+name+r'\([^)]*\)\n\{.*?^}',s,re.M|re.S).group())
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <new>
using addr_t=uintptr_t;using uint32=uint32_t;using status_t=int;
constexpr int B_OK=0,B_NO_MEMORY=-1;
#define ERROR(...) ((void)0)
#define TRACE(...) ((void)0)
static bool failHash;
static int live;
struct aperture_memory {addr_t base;size_t size;uint32 flags;size_t bound_size;void* pages;aperture_memory* next;aperture_memory(){live++;}~aperture_memory(){live--;}};
struct Hash {aperture_memory* value=nullptr;int Insert(aperture_memory* m){if(failHash)return B_NO_MEMORY;value=m;return B_OK;}};
struct Aperture {
 Hash fHashTable;aperture_memory* fFirstMemory=nullptr;
 int _Insert(aperture_memory* m,size_t size,size_t,uint32){m->base=0x100000;m->size=size;m->next=fFirstMemory;fFirstMemory=m;return B_OK;}
 void _Remove(aperture_memory*);
 aperture_memory* CreateMemory(size_t,size_t,uint32);
};
'''
suffix=r'''
int main(){
 Aperture a;aperture_memory existing;existing.next=nullptr;a.fFirstMemory=&existing;
 for(int n=0;n<100;n++) {
  failHash=true;
  assert(a.CreateMemory(4096,0,7)==nullptr);
  assert(live==1&&a.fFirstMemory==&existing&&existing.next==nullptr&&a.fHashTable.value==nullptr);
 }
 failHash=false;auto* m=a.CreateMemory(4096,0,7);
 assert(m&&live==2&&a.fHashTable.value==m&&a.fFirstMemory==m&&m->next==&existing);
 assert(m->flags==7&&m->bound_size==0&&m->pages==nullptr);
 a._Remove(m);a.fHashTable.value=nullptr;delete m;
 assert(live==1&&a.fFirstMemory==&existing);
 puts("100 failed metadata publications and subsequent successful publication/cleanup passed");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix+'\n'.join(functions)+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
