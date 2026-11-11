#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
using uint32=uint32_t;using uint64=uint64_t;using status_t=int;using cpu_status=int;using spinlock=std::mutex;
#define B_SPINLOCK_INITIALIZER {}
#define B_OK 0
#define B_ERROR -1
#define B_URGENT_DISPLAY_PRIORITY 0
static int disable_interrupts(){return 0;}static void restore_interrupts(int){}
static void acquire_spinlock(spinlock* l){l->lock();}static void release_spinlock(spinlock* l){l->unlock();}
struct dpc_module_info{status_t(*new_dpc_queue)(void**,const char*,int);status_t(*queue_dpc)(void*,void(*)(void*),void*);};
static status_t create(void** p,const char*,int){*p=(void*)1;return B_OK;}
static status_t enqueue(void*,void(*)(void*),void*);
static dpc_module_info module={create,enqueue};static dpc_module_info* gDPC=&module;static void* gDPCHandle;
#include "production.inc"
struct Pending{void(*fn)(void*);void* arg;};static std::vector<Pending> pending;
static int mode=0,calls=0,completed=0;static std::mutex gate;static std::condition_variable cv;static bool entered=false,proceed=false;
static status_t enqueue(void*,void(*fn)(void*),void* arg){calls++;
 if(mode==1)return B_ERROR;
 if(mode==2){std::thread t([=]{fn(arg);});t.join();assert(((ACPIDPCCallback*)arg)->references==1);return B_OK;}
 if(mode==3){std::unique_lock<std::mutex> l(gate);entered=true;cv.notify_all();cv.wait(l,[]{return proceed;});}
 pending.push_back({fn,arg});return B_OK;
}
static void callback(void*){completed++;}
static void nested(void*){assert(acpi_queue_callback(callback,nullptr)==B_OK);completed++;}
static void executing(void*){std::unique_lock<std::mutex> l(gate);entered=true;cv.notify_all();cv.wait(l,[]{return proceed;});completed++;}
static void waitEntered(){std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
static void resume(){std::lock_guard<std::mutex> l(gate);proceed=true;cv.notify_all();}
int main(int argc,char** argv){assert(argc==2);assert(acpi_initialize_dpc()==B_OK);int c=argv[1][0]-'0';
 if(c==0){mode=2;assert(acpi_queue_callback(callback,nullptr)==B_OK);assert(completed==1);}
 if(c==1){mode=1;assert(acpi_queue_callback(callback,nullptr)==B_ERROR);assert(sDPCCallbackCount==0);}
 if(c==2){for(int i=0;i<64;i++)assert(acpi_queue_callback(callback,nullptr)==B_OK);assert(acpi_queue_callback(callback,nullptr)==B_ERROR);assert(calls==64);acpi_close_dpc_admission();assert(sDPCCallbackCount==64);for(auto p:pending)p.fn(p.arg);}
 if(c==3){mode=3;std::thread t([]{assert(acpi_queue_callback(callback,nullptr)==B_OK);});waitEntered();assert(acpi_close_dpc_admission()==1);assert(acpi_queue_callback(callback,nullptr)==B_ERROR);resume();t.join();assert(sDPCCallbackCount==1);pending[0].fn(pending[0].arg);}
 if(c==4){assert(acpi_queue_callback(executing,nullptr)==B_OK);auto p=pending[0];std::thread t([=]{p.fn(p.arg);});waitEntered();assert(acpi_close_dpc_admission()==0);assert(sDPCCallbackCount==1);resume();t.join();}
 if(c==5){assert(acpi_queue_callback(nested,nullptr)==B_OK);auto p=pending[0];p.fn(p.arg);assert(sDPCCallbackCount==1);pending[1].fn(pending[1].arg);assert(completed==2);}
 if(c==6){sDPCProducers=~uint32(0);assert(acpi_queue_callback(callback,nullptr)==B_ERROR);assert(calls==0);sDPCProducers=0;}
 if(c==7){acpi_close_dpc_admission();assert(acpi_queue_callback(callback,nullptr)==B_ERROR);assert(calls==0);}
 assert(sDPCCallbackCount==0 && sDPCProducers==0);for(auto& r:sDPCCallbacks)assert(r.references==0);
}
