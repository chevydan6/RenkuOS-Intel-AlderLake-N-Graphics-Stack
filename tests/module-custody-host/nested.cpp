#include <initializer_list>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using thread_id=int;
thread_id thread_get_current_thread_id(){return 1;}
using status_t=int; using int32=int; using image_id=int;
const int B_OK=0,B_NO_ERROR=0,B_ERROR=-1,B_BUSY=-2,B_BAD_VALUE=-3,B_ENTRY_NOT_FOUND=-4,B_NO_MEMORY=-5;
const int B_KEEP_LOADED=1,B_BUILT_IN_MODULE=2,B_MODULE_INIT=1,B_MODULE_UNINIT=2;
enum {MODULE_QUERIED,MODULE_LOADED,MODULE_INIT,MODULE_UNINIT,MODULE_READY,MODULE_ERROR};
struct module_info {int (*std_ops)(int,...);};
struct module_dependency {const char* name;module_info** info;};
struct module_image;
struct module {const char* name; int state,flags,ref_count; module_info* info; struct module_image* module_image; int offset; bool lifecycle_retained=false; unsigned long long transition_generation=0; bool transition_active=false;};
module* entries[5]; int callbacks,images,searches,panics,failAt=-1;bool failAlloc,buildingProbe,failInit,nestedAcquire;int gBootDevice=-1,sModulesLock=0;
int recursive_lock_get_recursion(int* lock){return *lock ? *lock : -1;}
void recursive_lock_lock(int* lock){++*lock;}
void recursive_lock_unlock(int* lock){assert(*lock>0);--*lock;}
struct RecursiveLocker { int& lock; explicit RecursiveLocker(int& value):lock(value){++lock;} ~RecursiveLocker(){--lock;} };
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
using uint32=uint32_t;using uint64=uint64_t;using ACPI_STATUS=int;using spinlock=int;using cpu_status=int;
#define B_SPINLOCK_INITIALIZER 0
#define B_OK 0
#define B_ERROR -1
#define AE_OK 0
#define B_MODULE_INIT 1
#define B_MODULE_UNINIT 2
#define B_URGENT_DISPLAY_PRIORITY 0
#define TRUE 1
#define ACPI_FULL_INITIALIZATION 0
#define ACPI_DISABLE_ALL_VENDOR_STRINGS 0
#define B_SAFEMODE_DRIVER_SETTINGS "safe"
#define B_SAFEMODE_DISABLE_ACPI "disable"
#define ENOSYS 38
#define ERROR(...) ((void)0)
#undef TRACE
#define TRACE(...) ((void)0)
using ACPI_STRING=char*;
static int calls, creates, locked,archCalls,probeCalls;
static bool AcpiGbl_CopyDsdtLocally;
static int disable_interrupts(){return 0;}static void restore_interrupts(int){assert(!locked);}
static void acquire_spinlock(int*){assert(!locked);locked=1;}static void release_spinlock(int*){assert(locked);locked=0;}
static int step(){assert(sModulesLock==0);++calls;return calls==failAt?77:AE_OK;}
static int AcpiInitializeSubsystem(){return step();}static int AcpiInitializeTables(void*,int,int){return step();}static int AcpiLoadTables(){return step();}static int AcpiEnableSubsystem(int){return step();}static int AcpiInitializeObjects(int){return step();}
static int AcpiEnableAllRuntimeGpes(){return failAt==8?88:0;}static int AcpiUpdateAllGpes(){return failAt==9?99:0;}
static int AcpiUpdateInterfaces(int){return 0;}static int AcpiInstallInterface(char*){return 0;}
static void arch_init_interrupt_controller(){++archCalls;}static void embedded_controller_probe_ecdt(){++probeCalls;}
static bool checkAndLogFailure(int s,const char*){return s!=0;}
static void* load_driver_settings(const char*){return nullptr;}static bool get_driver_boolean_parameter(void*,const char*,bool a,bool){return a;}static void unload_driver_settings(void*){}
struct smbios_module_info {bool (*match_vendor_product)(const char*,void*);};
#define SMBIOS_MODULE_NAME "smbios"

struct dpc_module_info {int (*new_dpc_queue)(void**,const char*,int);};
static int create(void** q,const char*,int){++creates;*q=&creates;return failAt==6?-7:0;}
static dpc_module_info dependency{create};static dpc_module_info* gDPC=&dependency;static void* gDPCHandle;

#include "lifecycle.inc"
#define B_ACPI_MODULE_NAME "core"
#include "root.inc"
static int dependency_ops(int op,...){assert(sModulesLock==(op==B_MODULE_INIT?0:1));return B_OK;}
int main(int argc,char** argv){assert(argc==2);failAt=atoi(argv[1]);
 module_info rootInfo{acpi_module_std_ops},coreInfo{acpi_std_ops},depInfo{dependency_ops};
 module_info* infos[]={&rootInfo,&coreInfo};module_info* slots[3]={};
 module_dependency deps[]={{"a",&slots[0]},{"b",&slots[1]},{"c",&slots[2]},{nullptr,nullptr}};
 module_image image{};image.info=infos;image.dependencies=deps;
 module root{"root",MODULE_LOADED,0,0,&rootInfo,&image,0},core{"core",MODULE_LOADED,0,0,&coreInfo,&image,1};
 module a{"a",MODULE_LOADED,B_BUILT_IN_MODULE,0,&depInfo,nullptr,0},b{"b",MODULE_LOADED,B_BUILT_IN_MODULE,0,&depInfo,nullptr,0},c{"c",MODULE_LOADED,B_BUILT_IN_MODULE,0,&depInfo,nullptr,0};
 entries[0]=&root;entries[1]=&core;entries[2]=&a;entries[3]=&b;entries[4]=&c;
 module_info* out=nullptr;bool good=failAt==0||failAt>=7;
 assert(get_module("root",&out)==(good?B_OK:B_ERROR));
 assert(out==(good?&rootInfo:nullptr));assert(image.dependency_leases==2&&image.ref_count==2);
 assert(root.ref_count==(good?1:0)&&core.ref_count==(good?1:0)&&creates==1&&gDPCHandle==&creates);
 for(int i=0;i<3;i++)assert(slots[i]==&depInfo&&entries[i+2]->ref_count==1&&image.dependency_records[i]==&depInfo);
 if(good){if(failAt==7){dpc_module_info* d;void* q;assert(acpi_acquire_dpc(&d,&q));assert(sDPCProducers==1);}
 assert(put_module("root")==B_ERROR);assert(sDPCProducers==(failAt==7?1u:0u));}
 assert(root.state==MODULE_ERROR&&core.state==MODULE_ERROR&&image.dependency_state==DependenciesQuarantined);
 assert(sACPILifecycle==ACPIRetained);if(failAt==6)assert(sACPIDPCFailure==-7);
 int old=calls;for(int i=0;i<3;i++){out=nullptr;assert(get_module("root",&out)==B_ERROR&&out==nullptr);assert(get_module("core",&out)==B_ERROR&&out==nullptr);assert(put_module("root")==B_ERROR&&put_module("core")==B_ERROR);assert(acpi_std_ops(B_MODULE_INIT)==B_ERROR&&acpi_std_ops(B_MODULE_UNINIT)==B_ERROR);}
 assert(calls==old&&creates==1&&images==0&&image.ref_count==2&&image.dependency_leases==2&&root.ref_count==0&&root.lifecycle_retained&&core.ref_count==0&&core.lifecycle_retained&&gDPCHandle==&creates);
 std::free(image.dependency_records);puts("actual root/core lifecycle: retained references, leases, unpublished failure and no replay passed");}
