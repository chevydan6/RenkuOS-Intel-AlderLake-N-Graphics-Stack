#include <initializer_list>
#include <cassert>
#include <cstdint>
#include <cstdio>
using status_t=int; using int32=int;
const int B_OK=0,B_NO_ERROR=0,B_ERROR=-1,B_BUSY=-2,B_BAD_VALUE=-3,B_ENTRY_NOT_FOUND=-4;
const int B_KEEP_LOADED=1,B_BUILT_IN_MODULE=2,B_MODULE_INIT=1,B_MODULE_UNINIT=2;
enum {MODULE_QUERIED,MODULE_LOADED,MODULE_INIT,MODULE_UNINIT,MODULE_READY,MODULE_ERROR};
struct module_info {int (*std_ops)(int);};
struct module_image {int ref_count; module_info** info;};
struct module {const char* name; int state,flags,ref_count; module_info* info; struct module_image* module_image; int offset;};
module* current; int callbacks,dependencies,images,searches,panics; bool failUninit,reenter;
int gBootDevice=1,sModulesLock=0;
struct RecursiveLocker { explicit RecursiveLocker(int&) {} };
struct Hash {module* Lookup(const char*) {return current;}} hash;
Hash* sModulesHash=&hash;
#define TRACE(x) ((void)0)
#define FATAL(x) ((void)0)
#define ASSERT(x) assert(x)
template<class... T> void panic(const char*,T...) {++panics;}
int atomic_add(int* p,int n) {int old=*p;*p+=n;return old;}
void put_module_image(module_image* p) {++images; --p->ref_count;}
int get_dependent_modules(module*) {++dependencies; return B_OK;}
void put_dependent_modules(module*) {--dependencies;}
module* search_module(const char*,module_image**) {++searches;return nullptr;}
status_t get_module(const char*,module_info**); status_t put_module(const char*);
#include "production.inc"
int callback(int op) {
 ++callbacks;
 if(reenter) {module_info* out=reinterpret_cast<module_info*>(uintptr_t(1)); int refs=current->ref_count;
  assert(get_module("test",&out)==B_BUSY); assert(put_module("test")==B_BUSY);
  assert(out==reinterpret_cast<module_info*>(uintptr_t(1)));assert(current->ref_count==refs);}
 return op==B_MODULE_UNINIT&&failUninit?B_ERROR:B_OK;
}
int main() {
 module_info info{callback};module_info* infos[]={&info}; module_image image{1,infos};
 module m{"test",MODULE_READY,0,1,&info,&image,0};current=&m;dependencies=1;failUninit=true;reenter=true;
 assert(put_module("test")==B_OK); assert(m.state==MODULE_ERROR&&m.ref_count==1&&callbacks==1);
 for(int i=0;i<8;i++){module_info* out=nullptr;assert(get_module("test",&out)==B_ERROR);assert(out==nullptr);assert(put_module("test")==B_ERROR);}
 assert(m.ref_count==1&&image.ref_count==1&&dependencies==1&&images==0&&callbacks==1);
 for(int state:{MODULE_ERROR,MODULE_INIT,MODULE_UNINIT}) {
  for(int refs:{0,1,3}) {m.state=state;m.ref_count=refs;gBootDevice=-1;
   module_info* out=reinterpret_cast<module_info*>(uintptr_t(1));
   assert(get_module("test",&out)==(state==MODULE_ERROR?B_ERROR:B_BUSY));
   assert(put_module("test")==(state==MODULE_ERROR?B_ERROR:B_BUSY));
   assert(m.ref_count==refs&&out==reinterpret_cast<module_info*>(uintptr_t(1))&&image.ref_count==1);}}
 assert(callbacks==1&&dependencies==1&&images==0&&searches==0&&panics==0);
 gBootDevice=1;failUninit=false;m.state=MODULE_READY;m.flags=0;m.ref_count=1;
 module_info* out=nullptr;assert(get_module("test",&out)==B_OK&&out==&info&&m.ref_count==2);
 assert(put_module("test")==B_OK&&m.ref_count==1&&callbacks==1);
 assert(put_module("test")==B_OK&&m.ref_count==0&&m.state==MODULE_LOADED&&m.module_image==nullptr);
 assert(callbacks==2&&dependencies==0&&images==1&&panics==0);
 m.module_image=&image;image.ref_count=1;m.state=MODULE_LOADED;m.flags=B_KEEP_LOADED;m.ref_count=0;
 assert(get_module("test",&out)==B_OK&&m.ref_count==2&&m.state==MODULE_READY);
 assert(put_module("test")==B_OK&&m.ref_count==1);
 assert(get_module("test",&out)==B_OK&&m.ref_count==2);assert(put_module("test")==B_OK&&m.ref_count==1);
 assert(callbacks==3&&dependencies==1&&images==1&&panics==0);
 puts("failed UNINIT quarantine, 9 state/count pairs, INIT/UNINIT reentry, READY balance, teardown and permanent residency passed");
}
