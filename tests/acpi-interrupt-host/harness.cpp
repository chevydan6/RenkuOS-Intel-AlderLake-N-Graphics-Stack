#include <initializer_list>
#include <cstring>
#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <cstdio>
#define _KERNEL_MODE
using UINT32=uint32_t; using ACPI_STATUS=int; using ACPI_OSD_HANDLER=uint32_t(*)(void*); using interrupt_handler=ACPI_OSD_HANDLER;
using status_t=int; using cpu_status=int; using spinlock=std::mutex;
#define B_SPINLOCK_INITIALIZER {}
#define NUM_IO_VECTORS 224
#define B_OK 0
#define AE_OK 0
#define AE_ERROR 1
#define AE_BAD_PARAMETER 2
#define DEBUG_FUNCTION_F(...)
cpu_status disable_interrupts(){return 0;} void restore_interrupts(cpu_status){}
thread_local int held=0;
void acquire_spinlock(spinlock* s){s->lock();held++;} void release_spinlock(spinlock* s){held--;s->unlock();}
std::mutex gate; std::condition_variable cv; bool pauseAPI=false,entered=false,proceed=false;
int calls=0,answer=0; void* expected=nullptr;
int api(void* context){assert(held==0);assert(context==expected);std::unique_lock<std::mutex> l(gate);calls++;entered=true;cv.notify_all();if(pauseAPI)cv.wait(l,[]{return proceed;});return answer;}
int install_io_interrupt_handler(int,interrupt_handler,void* c,int){return api(c);}
int remove_io_interrupt_handler(int,interrupt_handler,void* c){return api(c);}
#include "production.inc"
uint32_t handler(void*){return 0;} uint32_t other(void*){return 0;}
int sACPILifecycle=0; const int ACPIRetained=1; const int B_ERROR=-1;
int dpcCloses=0; void acpi_close_dpc_admission(){assert(held==0);dpcCloses++;}
#include "lifecycle.inc"
void closeLifecycle(){assert(acpi_retain_lifecycle()==B_ERROR);assert(sACPILifecycle==ACPIRetained);assert(held==0);}
int main(int argc,char** argv){
 if(argc>1){
  int c=1;expected=&c;
  bool failure=std::strcmp(argv[1],"late-failure")==0;
  bool removing=std::strcmp(argv[1],"removing")==0;
  if(std::strcmp(argv[1],"closed")==0){
   assert(AcpiOsInstallInterruptHandler(7,handler,&c)==AE_OK);
   answer=-1;assert(AcpiOsRemoveInterruptHandler(7,handler)==AE_ERROR);
   closeLifecycle();closeLifecycle();int n=calls;
   assert(AcpiOsInstallInterruptHandler(8,handler,&c)==AE_ERROR);
   assert(AcpiOsRemoveInterruptHandler(7,handler)==AE_ERROR);
   assert(sInterrupts[7].context==&c && sInterrupts[7].phase==IRQUncertain);
   assert(calls==n && dpcCloses==2);puts("closed rejects fresh installs; quarantine retained");return 0;
  }
  if(removing)assert(AcpiOsInstallInterruptHandler(5,handler,&c)==AE_OK);
  answer=failure?-1:0;pauseAPI=true;entered=false;
  std::thread t([&]{assert((removing?AcpiOsRemoveInterruptHandler(5,handler):AcpiOsInstallInterruptHandler(5,handler,&c))==(failure?AE_ERROR:AE_OK));});
  {std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
  // Install above may set entered before the paused removal starts.
  if(removing){while(true){acquire_spinlock(&sInterruptLock);bool ready=sInterrupts[5].phase==IRQRemoving;release_spinlock(&sInterruptLock);if(ready)break;std::this_thread::yield();}}
  closeLifecycle();closeLifecycle();int n=calls;
  assert(sInterrupts[5].context==&c);
  assert(AcpiOsInstallInterruptHandler(6,handler,&c)==AE_ERROR);assert(calls==n);
  {std::lock_guard<std::mutex> l(gate);proceed=true;}cv.notify_all();t.join();pauseAPI=false;
  assert(sInterruptAdmissionClosed);
  assert(sInterrupts[5].phase==((failure||removing)?IRQEmpty:IRQInstalled));
  assert(AcpiOsInstallInterruptHandler(6,handler,&c)==AE_ERROR);
  if(!failure&&!removing){assert(sInterrupts[5].handler==handler && sInterrupts[5].context==&c);assert(AcpiOsRemoveInterruptHandler(5,other)==AE_ERROR);assert(AcpiOsRemoveInterruptHandler(5,handler)==AE_OK);}
  puts("overlap retained; completion cannot reopen admission");return 0;
 }

 int c=1,d=2; expected=&c;
 for(auto v:{224u,0xffffffffu}){assert(AcpiOsInstallInterruptHandler(v,handler,&c)==AE_BAD_PARAMETER);assert(AcpiOsRemoveInterruptHandler(v,handler)==AE_BAD_PARAMETER);}
 assert(AcpiOsInstallInterruptHandler(1,nullptr,&c)==AE_BAD_PARAMETER);assert(calls==0);
 answer=-1;assert(AcpiOsInstallInterruptHandler(223,handler,&c)==AE_ERROR);assert(sInterrupts[223].phase==IRQEmpty);
 answer=0;assert(AcpiOsInstallInterruptHandler(223,handler,&c)==AE_OK);int n=calls;
 assert(AcpiOsInstallInterruptHandler(223,other,&d)==AE_ERROR);assert(AcpiOsRemoveInterruptHandler(223,other)==AE_ERROR);assert(calls==n);
 assert(AcpiOsRemoveInterruptHandler(223,handler)==AE_OK);
 auto overlap=[&](bool removal){
  pauseAPI=true;entered=false;proceed=false;
  std::thread t([&]{assert((removal?AcpiOsRemoveInterruptHandler(5,handler):AcpiOsInstallInterruptHandler(5,handler,&c))==AE_OK);});
  {std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
  assert(AcpiOsInstallInterruptHandler(5,other,&d)==AE_ERROR);assert(AcpiOsRemoveInterruptHandler(5,handler)==AE_ERROR);assert(sInterrupts[5].context==&c);
  {std::lock_guard<std::mutex> l(gate);proceed=true;}cv.notify_all();t.join();pauseAPI=false;
 };
 overlap(false);overlap(true);
 assert(AcpiOsInstallInterruptHandler(7,handler,&c)==AE_OK);answer=-1;assert(AcpiOsRemoveInterruptHandler(7,handler)==AE_ERROR);n=calls;
 assert(AcpiOsRemoveInterruptHandler(7,handler)==AE_ERROR);assert(AcpiOsInstallInterruptHandler(7,other,&d)==AE_ERROR);assert(calls==n);assert(sInterrupts[7].context==&c);
 puts("bounds, identity, install rollback, removal quarantine and real-thread overlaps passed");
}
