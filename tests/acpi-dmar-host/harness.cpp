#include <cassert>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
using uint32=uint32_t;using UINT32=uint32_t;using int32=int;using status_t=int;using cpu_status=int;using spinlock=std::mutex;using ACPI_STATUS=uint32;
#define B_SPINLOCK_INITIALIZER {}
#define ACPI_FAILURE(x) ((x)!=0)
#define B_ACPI_MODULE_NAME "core"
const int B_MODULE_INIT=1,B_MODULE_UNINIT=2,B_BAD_VALUE=-2,B_ERROR=-1,AE_OK=0,ACPI_MTX_TABLES=3;
struct module_info {const char* name;int flags;int32 (*std_ops)(int32,...);};
int refs=0,dependencyError=0,acquires=0,releases=0;thread_local int held=0;
int get_module(const char* n,module_info**){assert(std::string(n)=="core");if(dependencyError)return dependencyError;refs++;return 0;}
int put_module(const char* n){assert(std::string(n)=="core");refs--;return 0;}
cpu_status disable_interrupts(){return 0;}void restore_interrupts(cpu_status){}
void acquire_spinlock(spinlock* s){s->lock();held++;}void release_spinlock(spinlock* s){held--;s->unlock();}
void acpi_close_dpc_admission(){assert(held==0);}void acpi_close_interrupt_admission(){assert(held==0);}
struct ACPI_TABLE_DESC {uintptr_t Address;union {char Ascii[4];} Signature;int Flags;void* Pointer;};
ACPI_TABLE_DESC descriptors[4];struct {UINT32 CurrentTableCount;ACPI_TABLE_DESC* Tables;} AcpiGbl_RootTableList={0,descriptors};
std::mutex gate;std::condition_variable cv;bool pauseAPI=false,entered=false,go=false;uint32 answer=0;
uint32 AcpiUtAcquireMutex(int m){assert(m==ACPI_MTX_TABLES&&held==0);std::unique_lock<std::mutex> l(gate);acquires++;entered=true;cv.notify_all();if(pauseAPI)cv.wait(l,[]{return go;});return answer;}
void AcpiUtReleaseMutex(int m){assert(m==ACPI_MTX_TABLES&&answer==0);releases++;}
#include "production.inc"
int main(int,char** argv){std::string c=argv[1];auto& entry=sDMARDescriptorModule;uint32 status=999;
 assert(std::string(entry.info.name)==ACPI_DMAR_DESCRIPTOR_MODULE_NAME);
 if(c=="dependency"){dependencyError=-77;assert(entry.info.std_ops(B_MODULE_INIT)==-77);assert(refs==0&&acquires==0);return 0;}
 assert(entry.info.std_ops(B_MODULE_INIT)==0&&refs==1);
 if(c=="unused"||c=="starting"||c=="retained"){if(c=="starting")sACPILifecycle=ACPIStarting;if(c=="retained"){acpi_retain_lifecycle();acpi_retain_lifecycle();}AcpiGbl_RootTableList.Tables=nullptr;AcpiGbl_RootTableList.CurrentTableCount=100;assert(entry.classify(&status)==ACPI_DMAR_NOT_READY&&status==0&&acquires==0);return 0;}
 sACPILifecycle=ACPIReady;
 if(c.substr(0,6)=="unique"||c=="duplicate"){AcpiGbl_RootTableList.CurrentTableCount=c=="duplicate"?2:1;for(unsigned i=0;i<AcpiGbl_RootTableList.CurrentTableCount;i++){descriptors[i].Address=1;memcpy(descriptors[i].Signature.Ascii,"DMAR",4);descriptors[i].Pointer=(void*)1;descriptors[i].Flags=c=="duplicate"?0:c.back()-'0';}}
 if(c=="mutex")answer=71;
 if(c=="overlap"){pauseAPI=true;std::thread t([&]{assert(entry.classify(&status)==ACPI_DMAR_ABSENT);});{std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}acpi_retain_lifecycle();acpi_retain_lifecycle();uint32 rejected;assert(entry.classify(&rejected)==ACPI_DMAR_NOT_READY&&acquires==1);{std::lock_guard<std::mutex> l(gate);go=true;}cv.notify_all();t.join();assert(releases==1&&refs==1);return 0;}
 auto expected=c=="mutex"?ACPI_DMAR_MUTEX_FAILURE:c=="duplicate"?ACPI_DMAR_DUPLICATE:c.substr(0,6)=="unique"?ACPI_DMAR_UNSUPPORTED:ACPI_DMAR_ABSENT;
 assert(entry.classify(&status)==expected&&status==answer);assert(acquires==1&&releases==(answer?0:1));assert(entry.info.std_ops(B_MODULE_UNINIT)==0&&refs==0);
}
