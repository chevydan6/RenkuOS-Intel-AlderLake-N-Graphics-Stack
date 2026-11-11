#include "IntelApertureLeaseState.h"
using uint64=uint64_t;
constexpr int B_OK=0,B_BUSY=-3,B_NO_MEMORY=-4,B_KEEP_LOADED=1;
using mutex=std::mutex;
static mutex sLock;
static IntelApertureLeaseState sState;
#include "IntelN100PciPower.h"
using uint8=uint8_t;
struct OwnerDevice { uint8 bus,device,function; uint64 base,size; };
static OwnerDevice sDevice;
static uint64 sObservationLease;
static IntelApertureLeaseState::Operation sObservationPin;
static IntelN100PciSnapshot sObservationSnapshot;
static int pciGets,gartGets,pciPuts,initCalls,pciReads;
static bool pciFail,gartFail,sRegistersUncached;
static module_info* sPCI;
static module_info* sGART;
static struct { int registers_area; } sResources;
#define B_PCI_MODULE_NAME "pci"
#define B_AGP_GART_MODULE_NAME "gart"
#define dprintf(...) ((void)0)
static void mutex_init(mutex*,const char*){initCalls++;}
static int put_module(const char* n){assert(std::string(n)=="pci");pciPuts++;return 0;}
static bool ReadConfig(void*,uint16_t,uint8_t,uint32_t& value){
 assert(sLock.try_lock());sLock.unlock();pciReads++;
 std::unique_lock<std::mutex> l(gate);entered=true;cv.notify_all();
 if(pauseAPI)cv.wait(l,[]{return go;});
 value=UINT32_MAX;return true;
}
enum ObservationPhase {ObservationIdle,ObservationChecking,ObservationActive};
static ObservationPhase sObservationPhase=ObservationIdle;
static bool sObservationBlocked;
static void mutex_lock(mutex* m){m->lock();}
static void mutex_unlock(mutex* m){m->unlock();}
static int providerGets;
static bool dependencyFail,nested;
static int get_module(const char*,module_info**);
#define INTEL_MEMORY_OWNER_MODULE_NAME "owner/v0"
#include "owner.inc"
#include "admission.inc"
static int get_module(const char* name,module_info** output){
 assert(sLock.try_lock());sLock.unlock();
 if(std::string(name)==B_PCI_MODULE_NAME){pciGets++;return pciFail?-78:0;}
 if(std::string(name)==B_AGP_GART_MODULE_NAME){gartGets++;return gartFail?-79:0;}
 if(std::string(name)==INTEL_MEMORY_OWNER_MODULE_NAME)return StandardOperations(B_MODULE_INIT);
 assert(std::string(name)==ACPI_DMAR_DESCRIPTOR_MODULE_NAME);providerGets++;
 if(nested){intel_memory_preflight_report r{};assert(sPreflightModule.preflight(&r)==B_BUSY);assert(!r.completed);}
 if(dependencyFail)return -77;
 assert(sDMARDescriptorModule.info.std_ops(B_MODULE_INIT)==0);
 *output=&sDMARDescriptorModule.info;return 0;
}
int main(int,char** argv){
 std::string c=argv[1];uint64 display=0;
 pciFail=c=="pci-failure";gartFail=c=="gart-failure";
 int initialized=sPreflightModule.info.std_ops(B_MODULE_INIT);
 if(pciFail||gartFail){assert(initialized==(pciFail?-78:-79));assert(providerGets==0&&initCalls==0&&pciGets==1&&gartGets==!pciFail&&pciPuts==gartFail);return 0;}
 assert(initialized==B_OK&&pciGets==1&&gartGets==1&&initCalls==1&&pciPuts==0);
 assert(sResources.registers_area==-1&&!sRegistersUncached);
 assert(StandardOperations(B_MODULE_UNINIT)==B_BUSY);
 assert(StandardOperations(999)==B_BAD_VALUE);
 assert(providerGets==0);
 assert(sPreflightModule.info.std_ops(B_MODULE_UNINIT)==B_BUSY);
 assert(sPreflightModule.info.std_ops(999)==B_BAD_VALUE);
 if(c=="not-ready")assert(acpi_retain_lifecycle()==B_ERROR);
 assert(sState.BeginInitialization(display)==IntelApertureLeaseState::Success);
 assert(sState.FinishInitialization(display,true)==IntelApertureLeaseState::Success);
 sACPILifecycle=c=="not-ready"?ACPIUnused:ACPIReady;
 if(c=="unique"||c=="duplicate"){
  AcpiGbl_RootTableList.CurrentTableCount=c=="unique"?1:2;
  for(unsigned i=0;i<AcpiGbl_RootTableList.CurrentTableCount;i++){memcpy(descriptors[i].Signature.Ascii,"DMAR",4);descriptors[i].Address=1;}
 }
 if(c=="mutex")answer=71;
 dependencyFail=c=="dependency";nested=c=="nested";
 if(c=="observation")sObservationPhase=ObservationChecking;
 intel_memory_preflight_report report{};int result=0;
 if(c=="observation-first"){
  sDevice.base=4096;sDevice.size=4096;pauseAPI=true;uint64 lease=0;IntelN100PciSnapshot snapshot{};
  std::thread t([&]{result=BeginObservationPhase(&lease,&snapshot,ObservationActive);});
  {std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
  assert(sPreflightModule.preflight(&report)==B_BUSY&&providerGets==0);
  {std::lock_guard<std::mutex> l(gate);go=true;}cv.notify_all();t.join();
  assert(result==B_ERROR&&pciReads>0);return 0;
 }
 if(c=="close"||c=="preflight-first"){
  pauseAPI=true;std::thread t([&]{result=sPreflightModule.preflight(&report);});
  {std::unique_lock<std::mutex> l(gate);cv.wait(l,[]{return entered;});}
  if(c=="preflight-first"){
   uint64 lease=0;IntelN100PciSnapshot snapshot{};
   assert(BeginObservationPhase(&lease,&snapshot,ObservationActive)==B_BUSY);
   assert(pciReads==0&&providerGets==1);
  }else assert(ReleaseDisplay(display)==0);
  {std::lock_guard<std::mutex> l(gate);go=true;}cv.notify_all();t.join();
  if(c=="preflight-first"){assert(result==B_ERROR&&report.completed&&!report.configured&&refs==1);return 0;}
  assert(result==B_BUSY&&!report.completed&&sPreflightUncertain&&refs==1);
  assert(sPreflightModule.preflight(&report)==B_BUSY&&providerGets==1);return 0;
 }
 result=sPreflightModule.preflight(&report);
 assert(!report.configured);
 if(c=="observation"){assert(result==B_BUSY&&providerGets==0);return 0;}
 assert(report.completed&&report.dependency_status==(dependencyFail?-77:0));
 assert(result==(dependencyFail?-77:B_ERROR));
 auto expected=c=="unique"?ACPI_DMAR_UNSUPPORTED:c=="duplicate"?ACPI_DMAR_DUPLICATE:c=="mutex"?ACPI_DMAR_MUTEX_FAILURE:(c=="not-ready"||dependencyFail)?ACPI_DMAR_NOT_READY:ACPI_DMAR_ABSENT;
 assert(report.classification==expected&&report.acpica_status==answer);
 if(c=="repeated")for(int i=0;i<8;i++)assert(sPreflightModule.preflight(&report)==B_ERROR);
 assert(providerGets==1&&refs==(dependencyFail?0:1));
}
