#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
using uint32=uint32_t; using status_t=int; using cpu_status=int;using spinlock=std::mutex;using ACPI_TABLE_HEADER=int;
#define B_SPINLOCK_INITIALIZER {}
const int B_OK=0,B_ERROR=-1,AE_OK=0;
thread_local int held=0;
cpu_status disable_interrupts(){return 0;}void restore_interrupts(cpu_status){}
void acquire_spinlock(spinlock* s){s->lock();held++;}void release_spinlock(spinlock* s){held--;s->unlock();}
void acpi_close_dpc_admission(){assert(held==0);}void acpi_close_interrupt_admission(){assert(held==0);}
std::mutex gate;std::condition_variable cv;bool pauseAPI=false,entered=false,go=false;int calls=0,answer=0,table=42;
int AcpiGetTable(char*,uint32,ACPI_TABLE_HEADER** out){assert(held==0);std::unique_lock<std::mutex> l(gate);calls++;entered=true;cv.notify_all();if(pauseAPI)cv.wait(l,[]{return go;});if(answer==0)*out=&table;return answer;}
#include "production.inc"
int main(int,char** argv){std::string c=argv[1];void* out=nullptr;
 if(c=="unused"||c=="starting"){if(c=="starting")sACPILifecycle=ACPIStarting;assert(get_table("TEST",1,&out)==B_ERROR);assert(!out&&calls==0);return 0;}
 sACPILifecycle=ACPIReady;
 if(c=="closed"){acpi_retain_lifecycle();acpi_retain_lifecycle();assert(get_table("TEST",1,&out)==B_ERROR);assert(!out&&calls==0);return 0;}
 answer=c=="failure"||c=="overlap-failure"?1:0;
 if(c=="ready"||c=="failure"){assert(get_table("TEST",1,&out)==(answer?B_ERROR:B_OK));assert(calls==1);assert(out==(answer?nullptr:&table));return 0;}
 pauseAPI=true;std::thread t([&]{assert(get_table("TEST",1,&out)==(answer?B_ERROR:B_OK));});
 {std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
 acpi_retain_lifecycle();acpi_retain_lifecycle();void* rejected=nullptr;assert(get_table("TEST",1,&rejected)==B_ERROR);assert(!rejected&&calls==1);
 {std::lock_guard<std::mutex> l(gate);go=true;}cv.notify_all();t.join();assert(sACPILifecycle==ACPIRetained);assert(out==(answer?nullptr:&table));assert(get_table("TEST",1,&rejected)==B_ERROR);assert(calls==1);
}
