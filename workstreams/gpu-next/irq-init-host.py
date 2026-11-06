#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Observe actual IRQ initialization under modeled external failures.
This baseline probe reports resource leaks; it is not hardware validation.
"""
import argparse
import pathlib
import re
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
parser.add_argument('--early-paths', action='store_true')
parser.add_argument('--msi', action='store_true')
parser.add_argument('--generation', type=int, choices=(4, 8, 12), default=12)
args = parser.parse_args()
source = args.source.read_text()
match = re.search(r'^static void\ninit_interrupt_handler\([^)]*\)\n\{.*?^}', source, re.M | re.S)
if not match:
    raise SystemExit('missing production IRQ initializer')
body = match.group()
helper = re.search(r'^static status_t\nrelease_msi\([^)]*\)\n\{.*?^}', source, re.M | re.S)
if helper:
    body = helper.group() + '\n' + body
uninit = re.search(r'^void\nintel_extreme_uninit\([^)]*\)\n\{.*?^}', source, re.M | re.S)
if not uninit:
    raise SystemExit('missing production teardown')
body += '\n' + uninit.group()
lexical = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"', '', body, flags=re.S)
names = set(re.findall(r'\b[A-Z][A-Z0-9_]+\b', lexical))
functions = set(re.findall(r'\b([A-Z][A-Z0-9_]+)\(', lexical))
reserved = {'B_OK', 'B_ERROR', 'NULL', 'TRACE', 'ERROR', 'CALLED'}
constants = 'enum { ' + ', '.join(f'{n}={i+16}' for i,n in enumerate(sorted(names-functions-reserved))) + ' };\n'
macros = ''.join(f'#define {n}(...) 1u\n' for n in sorted(functions-reserved))
prefix = r'''
#include <cstdio>
#include <cstdint>
#include <cassert>
using uint32=uint32_t;using status_t=int;using thread_id=int;
constexpr int B_OK=0,B_ERROR=-1;
#define TRACE(...) ((void)0)
#define ERROR(...) ((void)0)
#define CALLED(...) ((void)0)
struct thread_info {int team;};
struct Device {int Generation()const{return 12;}};
struct Shared {int vblank_sem,pch_info;};
struct PciInfo {int bus,device,function;struct{struct{int interrupt_pin,interrupt_line;}h0;}u;};
struct intel_info {Shared* shared_info;PciInfo* pci;Device device_type;int irq;bool use_msi,fake_interrupts;void* memory_owner;int registers_area,shared_area;bool msi_configured;};
static int fault,semId,legacyLine;
static bool semLive,configured,enabled,handlerLive;
static bool unsafeDelete;
static int teardownFault;
static int create_sem(int,const char*){if(fault==1)return -1;semLive=true;return semId;}
static int delete_sem(int id){assert(id==semId&&semLive);semLive=false;return 0;}
static int find_thread(void*){return 1;}
static int get_thread_info(int,thread_info* out){out->team=2;return fault==2?-1:0;}
static int set_sem_owner(int,int){return fault==3?-1:0;}
struct PCI {
 int get_msi_count(int,int,int){return fault==7?0:1;}
 int configure_msi(int,int,int,int,uint32* vector){if(fault==4)return -1;configured=true;*vector=32;return 0;}
 int enable_msi(int,int,int){assert(configured);if(fault==5)return -1;enabled=true;return 0;}
 int disable_msi(int,int,int){assert(enabled);if(teardownFault==1)return -1;enabled=false;return 0;}
 int unconfigure_msi(int,int,int){assert(configured);if(enabled)unsafeDelete=true;if(teardownFault==2)return -1;configured=false;return 0;}
};
static PCI pciModule;static PCI* gPCI=&pciModule;
using interrupt_handler=int(*)(void*);
static int gen8_interrupt_handler(void*){return 0;}
static int gen11_interrupt_handler(void*){return 0;}
static int intel_interrupt_handler(void*){return 0;}
static int install_io_interrupt_handler(int,interrupt_handler,void*,int){if(fault==6)return -1;handlerLive=true;return 0;}
static int remove_io_interrupt_handler(int,interrupt_handler,void*){assert(handlerLive);if(teardownFault==3)return -1;handlerLive=false;return 0;}
static void release_aperture(intel_info&){if(handlerLive)unsafeDelete=true;}
static void delete_area(int){if(handlerLive)unsafeDelete=true;}
static uint32 find_reg(intel_info&,uint32 reg){return reg;}
static void gen8_enable_interrupts(intel_info&,int,bool){}
static void gen8_enable_global_interrupts(intel_info&,bool){}
static void gen11_enable_global_interrupts(intel_info&,bool){}
static void g35_clear_interrupt_status(intel_info&,int){}
struct pipes {void SetPipe(int){}};
static void intel_enable_interrupts(intel_info&,pipes&,bool){}
static uint32 read32(intel_info&,uint32){return 0;}
static void write32(intel_info&,uint32,uint32){}
'''
test = r'''
int main(){
 const char* names[]={"normal","create-sem","thread-info","sem-owner","configure-msi","enable-msi","install-handler","no-msi"};
 int findings=0;
 int earlyFailures=0;
 int msiFailures=0;
 for(int f=0;f<8;f++)for(int line: {0,11}){
  fault=f;legacyLine=line;semId=10;teardownFault=0;
  semLive=configured=enabled=handlerLive=false;
  Shared shared={};PciInfo pci={};pci.u.h0.interrupt_pin=1;pci.u.h0.interrupt_line=line;
  intel_info info={&shared,&pci,{},0,false,false,nullptr,1,2,false};
  init_interrupt_handler(info);
  bool orphanMSI=configured && (!enabled || !handlerLive);
  bool missingFallback=!handlerLive && !info.fake_interrupts;
  bool unneededSem=semLive && !handlerLive;
  printf("%s legacy=%d handler=%d configured=%d enabled=%d sem=%d fake=%d orphan-msi=%d missing-fallback=%d unused-sem=%d\n",
   names[f],line,handlerLive,configured,enabled,semLive,info.fake_interrupts,orphanMSI,missingFallback,unneededSem);
  if(orphanMSI||missingFallback||unneededSem)findings++;
  if(orphanMSI||configured!=info.msi_configured||enabled!=info.use_msi)msiFailures++;
  if(f==1||f==2||f==3||(f==7&&line==0)){
   if(handlerLive||configured||enabled||semLive||!info.fake_interrupts)earlyFailures++;
  }
 }
 for(int id: {0,10})for(int failure=0;failure<4;failure++){
  fault=0;semId=id;teardownFault=failure;
  semLive=configured=enabled=handlerLive=unsafeDelete=false;
  Shared shared={};PciInfo pci={};
  intel_info info={&shared,&pci,{},0,false,false,nullptr,1,2,false};
  init_interrupt_handler(info);
  intel_extreme_uninit(info);
  printf("teardown sem-id=%d fault=%d handler=%d configured=%d enabled=%d sem=%d unsafe-delete=%d\n",
   id,failure,handlerLive,configured,enabled,semLive,unsafeDelete);
  if(handlerLive||configured||enabled||semLive||unsafeDelete)findings++;
  if(failure==0&&(handlerLive||configured||enabled||unsafeDelete))earlyFailures++;
  if(failure!=3){
   if(configured!=info.msi_configured||enabled!=info.use_msi||unsafeDelete)msiFailures++;
   if(failure==0&&(configured||enabled))msiFailures++;
   if(failure==1&&(!configured||!enabled))msiFailures++;
   if(failure==2&&(!configured||enabled))msiFailures++;
  }
 }
 // Failed cleanup must leave actionable state; a later explicit retry may
 // clear it only after the backend succeeds. No interrupt handler exists here.
 for(int acquisitionFault: {5,6})for(int cleanupFault: {1,2}){
  fault=acquisitionFault;teardownFault=cleanupFault;semId=10;
  semLive=configured=enabled=handlerLive=unsafeDelete=false;
  Shared shared={};PciInfo pci={};pci.u.h0.interrupt_pin=1;pci.u.h0.interrupt_line=11;
  intel_info info={&shared,&pci,{},0,false,false,nullptr,1,2,false};
  init_interrupt_handler(info);
  bool retained=acquisitionFault==6||cleanupFault==2;
  bool enabledExpected=acquisitionFault==6&&cleanupFault==1;
  if(retained){
   if(!configured||!info.msi_configured||enabled!=enabledExpected
    ||info.use_msi!=enabledExpected||handlerLive||semLive||unsafeDelete)msiFailures++;
  }else if(configured||info.msi_configured||enabled||info.use_msi)msiFailures++;
  teardownFault=0;
#ifdef HAS_RELEASE_MSI
  if(release_msi(info)!=B_OK||configured||enabled||info.msi_configured||info.use_msi)msiFailures++;
#endif
 }
 printf("Observed %d cases with incomplete IRQ resource/fallback state (modeled host APIs only).\n",findings);
 printf("Early admission and valid semaphore teardown: %d failures\n",earlyFailures);
 printf("MSI acquisition and checked cleanup: %d failures\n",msiFailures);
#ifdef FOCUSED_MSI
 return msiFailures?2:0;
#elif defined(FOCUSED_EARLY_PATHS)
 return earlyFailures?2:0;
#else
 return findings?2:0;
#endif
}
'''
with tempfile.TemporaryDirectory(prefix='renku-irq-init-') as tmp:
    cpp=pathlib.Path(tmp)/'probe.cpp'; binary=pathlib.Path(tmp)/'probe'
    cpp.write_text(('#define HAS_RELEASE_MSI\n' if helper else '')+'#include <initializer_list>\n'+prefix.replace('return 12;', f'return {args.generation};')+constants+macros+body+test)
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*(['-DFOCUSED_MSI'] if args.msi else []),*(['-DFOCUSED_EARLY_PATHS'] if args.early_paths else []),str(cpp),'-o',str(binary)],check=True)
    sys.exit(subprocess.run([str(binary)],timeout=10).returncode)
