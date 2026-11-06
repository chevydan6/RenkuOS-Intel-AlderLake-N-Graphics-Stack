#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Run complete production driver init/uninit with modeled OS/owner/hardware.
Register constants are symbolic test values: this verifies lifetime control flow,
not register addresses, timing, interrupt hardware, clone semantics or scanout.
"""
import pathlib,re,subprocess,sys,tempfile
source=pathlib.Path(sys.argv[1]).read_text()
methods=[]
for result,name in [('static void','release_aperture'),('status_t','intel_free_memory'),('status_t','intel_allocate_memory'),('status_t','intel_extreme_init'),('void','intel_extreme_uninit')]:
 match=re.search(r'^'+result+r'\n'+name+r'\([^)]*\)\n\{.*?^}',source,re.M|re.S)
 if not match:raise SystemExit('missing '+name)
 methods.append(match.group())
body='\n\n'.join(methods)
# Keep every production statement; substitute only external types/services.
lexical=re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"','',body,flags=re.S)
names=set(re.findall(r'\b[A-Z][A-Z0-9_]+\b',lexical))
reserved={'NULL','ERROR','TRACE','CALLED','REGISTER_BLOCK','ROUND_TO_PAGE_SIZE','B_OK','B_BUSY','B_ANY_KERNEL_ADDRESS','B_PAGE_SIZE','B_FULL_LOCK','B_KERNEL_READ_AREA','B_KERNEL_WRITE_AREA','B_CLONEABLE_AREA','INTEL_MEMORY_OWNER_MODULE_NAME','ICL_DSSM_REF_FREQ_SHIFT'}
names.update(re.findall(r'\b(?:PCI_|INTEL_FAMILY_)[A-Za-z0-9_]+',lexical))
constants='enum { '+', '.join(f'{n}={i+16}' for i,n in enumerate(sorted(names-reserved)))+' };\n'
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
using status_t=int;using area_id=int;using aperture_id=int;using uint8=uint8_t;
using uint32=uint32_t;using uint64=uint64_t;using addr_t=uintptr_t;using phys_addr_t=uint64_t;
constexpr int B_OK=0,B_BUSY=-16,B_PAGE_SIZE=4096,B_ANY_KERNEL_ADDRESS=0,B_FULL_LOCK=0;
constexpr int B_KERNEL_READ_AREA=1,B_KERNEL_WRITE_AREA=2,B_CLONEABLE_AREA=4,ICL_DSSM_REF_FREQ_SHIFT=0;
#define ERROR(...) ((void)0)
#define TRACE(...) ((void)0)
#define CALLED() ((void)0)
#define REGISTER_BLOCK(x) (x)
#define ROUND_TO_PAGE_SIZE(x) (((x)+4095)&~size_t(4095))
#define INTEL_MEMORY_OWNER_MODULE_NAME "owner"
'''
support=r'''
struct DeviceType {
 int generation;bool n100;
 int Generation() const{return generation;}
 bool InGroup(int group) const{return n100&&group==INTEL_GROUP_ALD;}
 bool InFamily(int) const{return false;}
 bool HasDDI() const{return false;}
};
struct edid1_info{int unused;};
struct ring_buffer{uint8* base;uint32 register_base,size;addr_t offset;};
struct overlay_registers{};
struct intel_shared_info {
 uint32 register_blocks[512];ring_buffer primary_ring_buffer;
 area_id registers_area;uint8* graphics_memory;phys_addr_t physical_graphics_memory;
 size_t graphics_memory_size;int frame_buffer,dpms_mode,min_brightness,pch_info;
 bool internal_crt_support,got_vbt,single_head_locked;DeviceType device_type;
 uint32 gpu_pci_vendor,gpu_pci_device,gpu_pci_revision;
 struct{uint32 reference_frequency,max_frequency,min_frequency,divisor_register;}pll_info;
 char device_identifier[64];phys_addr_t physical_overlay_registers,physical_status_page,physical_cursor_memory;
 addr_t overlay_offset;addr_t status_page[4];uint8* cursor_memory;
 bool has_vesa_edid_info;edid1_info vesa_edid_info;
 uint32 fdi_link_frequency,hraw_clock,hw_cdclk;int vblank_sem;
};
struct pci_info {uint32 vendor_id,device_id,revision,bus,device,function;
 struct{struct{uint32 base_registers[6],base_register_sizes[6],base_register_flags[6];}h0;}u;};
struct aperture_info {phys_addr_t physical_base;addr_t base;size_t size,reserved_size;};
struct intel_memory_owner_resources {aperture_info aperture;area_id registers_area;addr_t registers;};
struct module_info{};
struct Owner;
struct intel_info {
 pci_info* pci;aperture_id aperture;addr_t aperture_base,registers;
 area_id registers_area,shared_area;intel_shared_info* shared_info;
 Owner* memory_owner;uint64 memory_owner_lease;bool memory_owner_ready;
 DeviceType device_type;int pch_info;const char* device_identifier;
 struct overlay_registers* overlay_registers;bool fake_interrupts,use_msi;uint8 irq;
};
static std::vector<std::string> events;
static int fault,refs,ownerReleases,legacyMaps,legacyUnmaps,mmioMaps,allocations;
static bool irqLive,msiLive,fakeIRQ;
static void* sharedMemory;
static bool sharedAlive,legacyMMIOAlive,ownerMMIOAlive;
static status_t delete_area(area_id id){
 if(id==1){assert(sharedAlive&&!irqLive&&!msiLive);events.push_back("delete-shared");sharedAlive=false;free(sharedMemory);sharedMemory=nullptr;}
 else if(id==2){assert(legacyMMIOAlive);events.push_back("delete-legacy-mmio");legacyMMIOAlive=false;}
 else {assert(id!=3&&"driver must not delete owner MMIO");}
 return B_OK;
}
struct AreaKeeper {
 int id=-1;
 ~AreaKeeper(){if(id>=0)delete_area(id);}
 area_id Create(const char*,void** out,int,size_t size,int,int){
  if(fault==3){return -5;}assert(!sharedAlive);sharedMemory=calloc(1,size);*out=sharedMemory;sharedAlive=true;return id=1;
 }
 area_id Map(const char*,phys_addr_t,uint64,int,int,void** out){
  mmioMaps++;if(fault==4)return -5;legacyMMIOAlive=true;*out=(void*)0x9000;return id=2;
 }
 status_t InitCheck()const{return id;}
 void Detach(){id=-1;}
};
struct Owner {
 status_t begin_display(uint32,uint32,uint32,uint64* lease,intel_memory_owner_resources* out){
  events.push_back("begin");if(fault==2)return -5;
  *lease=91;*out={{0x80000000,0x100000,0x10000,0x1000},3,0x9000};ownerMMIOAlive=true;return B_OK;
 }
 status_t finish_display(uint64 lease,bool success){
  assert(lease==91);events.push_back(success?"finish-ok":"finish-failed");
  if(fault==6)return B_BUSY;
  if(success&&fault==5)return -5;
  return B_OK;
 }
 status_t release_display(uint64 lease){assert(lease==91&&!irqLive&&!msiLive);events.push_back("release");ownerReleases++;return fault==6?B_BUSY:B_OK;}
 status_t allocate(uint64 lease,size_t,size_t,uint32,addr_t* base,phys_addr_t* physical){assert(lease==91);allocations++;if(fault==7)return -5;*base=0x102000;if(physical)*physical=0x40000;return B_OK;}
 status_t free(uint64 lease,addr_t){assert(lease==91);return B_OK;}
};
static Owner owner;
static status_t get_module(const char*,module_info** out){events.push_back("get");if(fault==1)return -5;refs++;*out=reinterpret_cast<module_info*>(&owner);return B_OK;}
static void put_module(const char*){events.push_back("put");refs--;assert(refs>=0);}
struct Gart {
 aperture_id map_aperture(uint32,uint32,uint32,size_t,addr_t* base){legacyMaps++;if(fault==8)return -5;*base=0x100000;return 7;}
 status_t unmap_aperture(aperture_id id){assert(id==7&&!irqLive);legacyUnmaps++;return B_OK;}
 status_t get_aperture_info(aperture_id id,aperture_info* out){assert(id==7);*out={0x80000000,0x100000,0x10000,0x1000};return B_OK;}
 status_t allocate_memory(aperture_id id,size_t,size_t,uint32,addr_t* base,phys_addr_t* physical){assert(id==7);allocations++;if(fault==7)return -5;*base=0x102000;if(physical)*physical=0x40000;return B_OK;}
 status_t free_memory(aperture_id id,addr_t){assert(id==7);return B_OK;}
};
static Gart gart;static Gart* gGART=&gart;
struct PCI {
 void set_powerstate(uint32,uint32,uint32,int){events.push_back("power");}
 void disable_msi(uint32,uint32,uint32){assert(msiLive&&!irqLive);events.push_back("disable-msi");msiLive=false;}
 void unconfigure_msi(uint32,uint32,uint32){assert(!msiLive);events.push_back("unconfigure-msi");}
};
static PCI pciModule;static PCI* gPCI=&pciModule;
static void read_settings(bool& cursor){cursor=true;}
static uint32 get_pci_config(pci_info*,int,int){return 0;}
static void set_pci_config(pci_info*,int,int,uint32){}
static void intel_en_gating(intel_info&){}
static void intel_en_downclock(intel_info&){}
static bool parse_vbt_from_bios(intel_shared_info*){return false;}
static uint32 read32(intel_info&,uint32){return 0;}
static void write32(intel_info&,uint32,uint32){}
static bool intel_uses_physical_overlay(const intel_shared_info&){return false;}
static void init_overlay_registers(overlay_registers*){}
static void* get_boot_item(int,void*){return nullptr;}
static void init_interrupt_handler(intel_info& info){
 events.push_back("irq-init");info.fake_interrupts=fakeIRQ;info.use_msi=!fakeIRQ;
 info.shared_info->vblank_sem=fakeIRQ?-1:10;irqLive=msiLive=!fakeIRQ;
}
using interrupt_handler=int(*)(void*);
static int gen8_interrupt_handler(void*){return 0;}
static int gen11_interrupt_handler(void*){return 0;}
static int intel_interrupt_handler(void*){return 0;}
static void gen11_enable_global_interrupts(intel_info&,bool enabled){assert(!enabled);events.push_back("disable-gen11");}
static void gen8_enable_global_interrupts(intel_info&,bool enabled){assert(!enabled);events.push_back("disable-gen8");}
static uint32 find_reg(const intel_info&,uint32 value){return value;}
static void remove_io_interrupt_handler(uint8,interrupt_handler,void*){assert(irqLive);irqLive=false;events.push_back("remove-irq");}
static status_t intel_allocate_memory(intel_info&,size_t,size_t,uint32,addr_t*,phys_addr_t* =nullptr);
void intel_extreme_uninit(intel_info&);
'''
suffix=r'''
int main(){
 unsigned cases=0;
 for(bool n100:{false,true})for(bool fallback:{false,true})for(int stage=0;stage<=8;stage++){
  assert(!sharedAlive&&!legacyMMIOAlive&&!irqLive&&!msiLive);
  events.clear();fault=stage;refs=ownerReleases=legacyMaps=legacyUnmaps=mmioMaps=allocations=0;
  fakeIRQ=fallback;ownerMMIOAlive=false;
  pci_info pci{};pci.vendor_id=0x8086;pci.device_id=n100?0x46d1:0x1234;
  pci.u.h0.base_registers[0]=0x90000000;pci.u.h0.base_register_sizes[0]=0x10000;
  intel_info info{};info.pci=&pci;info.device_type={n100?12:7,n100};
  info.pch_info=INTEL_PCH_NONE;info.device_identifier="test";
  status_t result=intel_extreme_init(info);
  bool shouldFail=n100?(stage==1||stage==2||stage==3||stage==5||stage==6)
                       :(stage==3||stage==4||stage==8);
  assert((result!=B_OK)==shouldFail);
  if(result==B_OK){
   assert(sharedAlive&&info.shared_info->registers_area==(n100?3:2));
   assert(info.memory_owner_ready==n100);
   assert(intel_free_memory(info,0x102000)==B_OK);
   intel_extreme_uninit(info);
  }
  assert(!sharedAlive&&!legacyMMIOAlive&&!irqLive&&!msiLive&&refs==0);
  if(n100){
   assert(legacyMaps==0&&legacyUnmaps==0&&mmioMaps==0);
   assert(ownerReleases==((stage==1||stage==2)?0:1));
   assert(ownerMMIOAlive==(stage!=1&&stage!=2));
  }else{
   assert(legacyMaps==1&&legacyUnmaps==(stage==8?0:1)&&ownerReleases==0);
   assert(!ownerMMIOAlive);
  }
  cases++;
 }
 printf("%u complete production driver init/uninit modeled lifecycle cases passed\n",cases);
}
'''
with tempfile.TemporaryDirectory(prefix='intel-driver-lifetime-') as directory:
 d=pathlib.Path(directory);(d/'test.cpp').write_text(prefix+constants+support+body+suffix)
 subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,timeout=30)
