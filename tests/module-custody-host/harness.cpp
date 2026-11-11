#include <initializer_list>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using status_t=int; using int32=int; using image_id=int;
const int B_OK=0,B_NO_ERROR=0,B_ERROR=-1,B_BUSY=-2,B_BAD_VALUE=-3,B_ENTRY_NOT_FOUND=-4,B_NO_MEMORY=-5;
const int B_KEEP_LOADED=1,B_BUILT_IN_MODULE=2,B_MODULE_INIT=1,B_MODULE_UNINIT=2;
enum {MODULE_QUERIED,MODULE_LOADED,MODULE_INIT,MODULE_UNINIT,MODULE_READY,MODULE_ERROR};
struct module_info {int (*std_ops)(int);};
struct module_dependency {const char* name;module_info** info;};
struct module_image;
struct module {const char* name; int state,flags,ref_count; module_info* info; struct module_image* module_image; int offset; bool lifecycle_retained=false;};
module* entries[5]; int callbacks,images,searches,panics,failAt=-1;bool failAlloc,buildingProbe,failInit,nestedAcquire;int gBootDevice=-1,sModulesLock=0;
struct RecursiveLocker { explicit RecursiveLocker(int&) {} };
struct Hash {module* Lookup(const char* n) {for(auto m:entries) if(m&&strcmp(n,m->name)==0)return m;return nullptr;}} hash;
Hash* sModulesHash=&hash;
#define TRACE(x) ((void)0)
#define FATAL(x) ((void)0)
#define ASSERT(x) assert(x)
template<class... T> void panic(const char*,T...) {++panics;}
int atomic_add(int* p,int n) {int old=*p;*p+=n;return old;}
void put_module_image(module_image*);module* search_module(const char*,module_image**);
status_t get_module(const char*,module_info**);status_t put_module(const char*);
void* test_calloc(size_t n,size_t s){return failAlloc?nullptr:std::calloc(n,s);}
#define calloc test_calloc
#include "production.inc"
#undef calloc
module_image* searchImage;module* searchTarget;
void put_module_image(module_image* p){++images;--p->ref_count;assert(p->ref_count>0);}
module* search_module(const char*,module_image** out){++searches;if(!searchTarget)return nullptr;*out=searchImage;++searchImage->ref_count;return searchTarget;}
int callback(int op){++callbacks;
 if(nestedAcquire&&op==B_MODULE_INIT&&entries[0]->state==MODULE_INIT&&entries[1]->state!=MODULE_INIT){module_info* out=nullptr;assert(get_module("core",&out)==B_OK);assert(entries[0]->module_image->dependency_leases==2);assert(put_module("core")==B_OK);assert(entries[0]->module_image->dependency_leases==1);}
 return op==B_MODULE_INIT&&failInit?B_ERROR:B_OK;}
int childCallback(int op){
 assert(op==B_MODULE_INIT);int i=entries[2]->state==MODULE_INIT?0:entries[3]->state==MODULE_INIT?1:2;
 if(buildingProbe){auto image=entries[0]->module_image;int refs=image->ref_count;module_info* out=nullptr;assert(get_module("core",&out)==B_BUSY&&out==nullptr);assert(put_module("root")==B_BUSY);assert(image->ref_count==refs);}
 return i==failAt?B_ERROR:B_OK;
}
int main(){
 for(int mode=-2;mode<5;mode++){
 callbacks=images=searches=panics=0;failAt=mode<3?mode:-1;failAlloc=mode==-2;buildingProbe=true;failInit=mode==4;nestedAcquire=mode==3;gBootDevice=-1;
 module_info parent{callback},child{childCallback};module_info* infos[]={&parent};module_info* slots[3]={};
 module_dependency deps[]={{"a",&slots[0]},{"b",&slots[1]},{"c",&slots[2]},{nullptr,nullptr}};
 module_image image{};image.info=infos;image.dependencies=deps;image.ref_count=0;
 module root{"root",MODULE_LOADED,0,0,&parent,&image,0},core{"core",MODULE_LOADED,0,0,&parent,&image,0};
 module a{"a",MODULE_LOADED,B_BUILT_IN_MODULE,0,&child,nullptr,0},b{"b",MODULE_LOADED,B_BUILT_IN_MODULE,0,&child,nullptr,0},c{"c",MODULE_LOADED,B_BUILT_IN_MODULE,0,&child,nullptr,0};
 entries[0]=&root;entries[1]=&core;entries[2]=&a;entries[3]=&b;entries[4]=&c;
 module_info* out=nullptr;int status=get_module("root",&out);
 if(mode==3){assert(status==B_OK&&callbacks==3&&image.dependency_leases==1&&core.ref_count==0&&image.ref_count==1);assert(put_module("root")==B_ERROR&&callbacks==4);}
 else if(mode==4){assert(status==B_ERROR&&out==nullptr&&root.ref_count==0&&root.lifecycle_retained&&image.ref_count==1&&callbacks==1&&image.dependency_state==DependenciesQuarantined);assert(put_module("root")==B_ERROR);assert(get_module("core",&out)==B_BUSY);}
 else if(mode!=-1){assert(status<0&&out==nullptr&&callbacks==0&&image.ref_count==1&&root.ref_count==0&&root.lifecycle_retained);assert(image.dependency_state==DependenciesQuarantined);int prefix=mode<0?0:mode;assert(image.dependency_acquired==size_t(prefix));for(int i=0;i<3;i++){assert(slots[i]==nullptr);if(image.dependency_records)assert((image.dependency_records[i]!=nullptr)==(i<prefix));assert(entries[i+2]->ref_count==(i<prefix?1:0));}
 for(int j=0;j<3;j++){assert(get_module("root",&out)==B_ERROR);assert(put_module("root")==B_ERROR);assert(get_module("core",&out)==B_BUSY);assert(put_module("core")==B_BUSY);assert(image.ref_count==1);}
 }else{assert(status==B_OK&&out==&parent&&image.dependency_leases==1&&callbacks==1);assert(get_module("core",&out)==B_OK&&image.dependency_leases==2&&image.ref_count==2&&callbacks==2);assert(put_module("root")==B_OK&&image.dependency_leases==1&&image.ref_count==1&&callbacks==3);for(int i=0;i<3;i++)assert(slots[i]==&child&&entries[i+2]->ref_count==1);
 assert(put_module("core")==B_ERROR&&image.dependency_state==DependenciesQuarantined&&core.ref_count==0&&core.lifecycle_retained&&callbacks==4);for(int j=0;j<3;j++){assert(put_module("core")==B_ERROR);assert(get_module("root",&out)==B_BUSY);}assert(image.ref_count==1&&callbacks==4);
 }
 // Rejected search must balance its temporary pin without publishing the parent.
 gBootDevice=1;core.module_image=nullptr;searchTarget=&core;searchImage=&image;int pins=image.ref_count;assert(get_module("core",&out)<0&&image.ref_count==pins&&core.module_image==nullptr);searchTarget=nullptr;
 assert(panics==0);std::free(image.dependency_records);
 }
 // Successful built-in behavior and genuine keep-loaded policy stay separate.
 for(int keep=0;keep<2;keep++) {
  module_info info{callback}; failInit=false;nestedAcquire=false;
  module m{"plain",MODULE_LOADED,B_BUILT_IN_MODULE|(keep?B_KEEP_LOADED:0),0,&info,nullptr,0};
  for(auto& entry:entries) {entry=nullptr;}
  entries[0]=&m;
  module_info* out=nullptr;int before=callbacks;
  assert(get_module("plain",&out)==B_OK&&out==&info);
  assert(get_module("plain",&out)==B_OK);assert(put_module("plain")==B_OK);
  assert(put_module("plain")==B_OK);assert(!m.lifecycle_retained);
  assert(m.ref_count==(keep?1:0));assert(callbacks==before+(keep?1:2));
 }
 puts("allocation/three failure positions, acquired prefixes, Building reentry, root/core reuse, intermediate/final release, replay and search pin balance passed");
}
