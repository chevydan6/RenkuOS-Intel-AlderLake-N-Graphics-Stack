# SPDX-License-Identifier: MIT
"""Exercise production aperture destructor lock release and backend ordering."""
import pathlib,re,subprocess,tempfile,sys
s=pathlib.Path(sys.argv[1]).read_text();body=re.search(r'^Aperture::~Aperture\(\)\n\{.*?^}',s,re.M|re.S).group()
prefix=r'''
#include <cassert>
#include <cstdio>
constexpr int B_OK=0;
#define ASSERT assert
struct TestLock {int sem;};
static unsigned lockDeletes,backendDeletes,puts;
static bool validLock;
[[maybe_unused]] static void uninit_lock(TestLock* lock){assert(lock->sem>=0);lockDeletes++;lock->sem=-1;}
static void Delete(void*){assert(lockDeletes==(validLock?1u:0u));assert(puts==0);backendDeletes++;}
static void put_module(const char*){assert(backendDeletes==1);puts++;}
struct Module {struct {const char* name;} info;void (*delete_aperture)(void*);};
struct Aperture {void* fFirstMemory=nullptr;TestLock fLock;Module* fModule;void* fPrivateAperture=nullptr;~Aperture();};
'''.replace('puts','modulePuts')
suffix=r'''
int main(){
 Module module{{"intel"},Delete};
 for(int trial=0;trial<200;trial++) {
  validLock=(trial%2)==0;lockDeletes=backendDeletes=modulePuts=0;
  auto* a=new Aperture;a->fLock.sem=validLock?73:-12;a->fModule=&module;
  delete a;
  assert(lockDeletes==(validLock?1u:0u)&&backendDeletes==1&&modulePuts==1);
 }
 printf("200 production aperture destructor valid/failed-lock cleanup cases passed\n");
}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d);(p/'test.cpp').write_text(prefix+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
