#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
using uint32=uint32_t;using uint64=uint64_t;using status_t=int;using int32=int;using ACPI_STATUS=int;using spinlock=int;using cpu_status=int;
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
#define TRACE(...) ((void)0)
using ACPI_STRING=char*;
static int failAt, calls, creates, locked,archCalls,probeCalls;
static bool AcpiGbl_CopyDsdtLocally;
static int disable_interrupts(){return 0;}static void restore_interrupts(int){assert(!locked);}
static void acquire_spinlock(int*){assert(!locked);locked=1;}static void release_spinlock(int*){assert(locked);locked=0;}
static int step(){++calls;return calls==failAt?77:AE_OK;}
static int AcpiInitializeSubsystem(){return step();}static int AcpiInitializeTables(void*,int,int){return step();}static int AcpiLoadTables(){return step();}static int AcpiEnableSubsystem(int){return step();}static int AcpiInitializeObjects(int){return step();}
static int AcpiEnableAllRuntimeGpes(){return failAt==8?88:0;}static int AcpiUpdateAllGpes(){return failAt==9?99:0;}
static int AcpiUpdateInterfaces(int){return 0;}static int AcpiInstallInterface(char*){return 0;}
static void arch_init_interrupt_controller(){++archCalls;}static void embedded_controller_probe_ecdt(){++probeCalls;}
static bool checkAndLogFailure(int s,const char*){return s!=0;}
static void* load_driver_settings(const char*){return nullptr;}static bool get_driver_boolean_parameter(void*,const char*,bool a,bool){return a;}static void unload_driver_settings(void*){}
struct module_info{};struct smbios_module_info {bool (*match_vendor_product)(const char*,void*);};
#define SMBIOS_MODULE_NAME "smbios"
static int get_module(const char*,module_info**){return -1;}static int put_module(const char*){return 0;}
struct dpc_module_info {int (*new_dpc_queue)(void**,const char*,int);};
static int create(void** q,const char*,int){++creates;*q=&creates;return failAt==6?-7:0;}
static dpc_module_info dependency{create};static dpc_module_info* gDPC=&dependency;static void* gDPCHandle;
#include "lifecycle.inc"
int main(int argc,char** argv){assert(argc==2);failAt=atoi(argv[1]);
 int result=acpi_std_ops(B_MODULE_INIT);
 bool good=failAt==0||failAt>=7;
 assert(result==(good?B_OK:B_ERROR));assert(creates==1&&gDPCHandle==&creates&&sDPCQueue==&creates&&sDPCDependency==gDPC);
 assert(sACPIAttempted==(failAt==6?1u:good?63u:((1u<<(failAt+1))-1)));
 assert(sACPICompleted==(failAt==6?0u:good?63u:((1u<<failAt)-1)));
 if(!good){assert(sACPILifecycle==ACPIRetained);assert(failAt==6?sACPIDPCFailure==-7:sACPIFailure==77);}
 else assert(sACPILifecycle==ACPIReady);
 if(good){assert(sACPINonfatalAttempted==3);assert(sACPINonfatalCompleted==(failAt==8?2u:failAt==9?1u:3u));assert(sACPINonfatalFailure==(failAt==8?88:failAt==9?99:0));}
 if(failAt==7)sDPCProducers=3;
 assert(acpi_std_ops(B_MODULE_UNINIT)==B_ERROR);assert(sACPILifecycle==ACPIRetained);
 assert(sDPCProducers==(failAt==7?3u:0u));int old=calls;
 for(int i=0;i<3;i++){assert(acpi_std_ops(B_MODULE_INIT)==B_ERROR);assert(acpi_std_ops(B_MODULE_UNINIT)==B_ERROR);}
 assert(calls==old&&creates==1&&gDPCHandle==&creates);dpc_module_info* d=nullptr;void* q=nullptr;assert(!acpi_acquire_dpc(&d,&q));assert(acpi_initialize_dpc()==B_ERROR);
 puts("actual acpi_std_ops retained failure/uninit/replay passed");
}
