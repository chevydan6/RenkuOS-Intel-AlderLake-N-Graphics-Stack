#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using uint32=uint32_t; using uint64=uint64_t; using status_t=int; using cpu_status=int; using spinlock=int;
using ACPI_STATUS=int; using ACPI_EXECUTE_TYPE=int; using ACPI_OSD_EXEC_CALLBACK=void(*)(void*);
#define B_SPINLOCK_INITIALIZER 0
#define B_OK 0
#define B_ERROR -1
#define B_URGENT_DISPLAY_PRIORITY 0
#define AE_OK 0
#define AE_BAD_PARAMETER 1
#define DEBUG_FUNCTION() ((void)type)
#define DEBUG_FUNCTION_F(...) ((void)0)
struct dpc_module_info {status_t (*new_dpc_queue)(void**,const char*,int); status_t (*queue_dpc)(void*,ACPI_OSD_EXEC_CALLBACK,void*);};
static int locked, preclose, calls, creates, closeInCreate, closeInQueue, reject, callbacks;
static status_t creationResult; static void* creationHandle;
bool acpi_acquire_dpc(dpc_module_info**,void**); void acpi_release_dpc(); uint32 acpi_close_dpc_admission();
static int disable_interrupts(){return 0;} static void restore_interrupts(int){assert(!locked);}
static void acquire_spinlock(int*) { if(preclose){preclose=0; assert(acpi_close_dpc_admission()==0);} assert(!locked); locked=1; }
static void release_spinlock(int*){assert(locked); locked=0;}
static status_t create(void** out,const char*,int){assert(!locked); creates++; if(closeInCreate)assert(acpi_close_dpc_admission()==0); *out=creationHandle; return creationResult;}
static status_t enqueue(void*,ACPI_OSD_EXEC_CALLBACK,void*);
static dpc_module_info module={create,enqueue}; static dpc_module_info* gDPC=&module; static void* gDPCHandle;
#include "production.inc"
static status_t enqueue(void* queue,ACPI_OSD_EXEC_CALLBACK fn,void* context){
 assert(!locked && sDPCProducers==1 && queue==creationHandle); calls++;
 if(closeInQueue){assert(acpi_close_dpc_admission()==1); assert(acpi_close_dpc_admission()==1);}
 // End-of-call hook models wakeup returning after publication, with the lease held.
 assert(sDPCProducers==1); if(reject)return B_ERROR;
 assert(fn && context==&callbacks); return B_OK;
}
static void callback(void*){callbacks++;}
int main(int argc,char** argv){
 assert(argc==2); const char* c=argv[1]; creationHandle=&module;
 if(!strcmp(c,"generation")){sDPCGeneration=~uint64(0); assert(acpi_initialize_dpc()==B_ERROR); assert(creates==0 && sDPCPhase==DPCQuarantined);}
 else if(!strcmp(c,"ambiguous") || !strcmp(c,"null") || !strcmp(c,"creating-close")){
  if(!strcmp(c,"ambiguous"))creationResult=B_ERROR;
  if(!strcmp(c,"null"))creationHandle=nullptr;
  if(!strcmp(c,"creating-close"))closeInCreate=1;
  assert(acpi_initialize_dpc()==B_ERROR); assert(sDPCQueue==creationHandle && gDPCHandle==creationHandle);
  assert(sDPCDependency==&module && sDPCPhase==DPCQuarantined);
  assert(acpi_initialize_dpc()==B_ERROR && creates==1);
 }else{
  assert(acpi_initialize_dpc()==B_OK && creates==1 && sDPCGeneration==1);
  if(!strcmp(c,"prelock"))preclose=1;
  if(!strcmp(c,"leased-close") || !strcmp(c,"wake"))closeInQueue=1;
  if(!strcmp(c,"reject"))reject=1;
  if(!strcmp(c,"postclose")){assert(acpi_close_dpc_admission()==0); gDPC=nullptr; sDPCDependency=nullptr; sDPCQueue=nullptr;}
  if(!strcmp(c,"counter"))sDPCProducers=~uint32(0);
  bool denied=!strcmp(c,"prelock")||!strcmp(c,"postclose")||!strcmp(c,"counter");
  assert(AcpiOsExecute(0,callback,&callbacks)==(denied||reject?AE_BAD_PARAMETER:AE_OK));
  assert(calls==(denied?0:1)); assert(callbacks==0);
  if(!strcmp(c,"counter")){assert(sDPCPhase==DPCQuarantined && sDPCProducers==~uint32(0));}
  else assert(sDPCProducers==0);
  auto count=acpi_close_dpc_admission(); assert(acpi_close_dpc_admission()==count);
  dpc_module_info* dep=&module; void* q=&callbacks;
  assert(!acpi_acquire_dpc(&dep,&q)); assert(dep==&module && q==&callbacks);
  assert(AcpiOsExecute(0,callback,&callbacks)==AE_BAD_PARAMETER); assert(calls==(denied?0:1));
  assert(acpi_initialize_dpc()==B_ERROR && creates==1);
 }
 puts(c);
}
