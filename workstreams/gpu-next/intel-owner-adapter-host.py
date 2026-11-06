#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compile actual retained-owner module against modeled OS/GART services."""
import pathlib, subprocess, sys, tempfile
root = pathlib.Path(sys.argv[1])
types = r'''
#pragma once
#include <cstdint>
#include <cstddef>
#include <mutex>
using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t;
using int32=int32_t; using status_t=int32_t; using area_id=int32_t;
using addr_t=uintptr_t; using phys_addr_t=uint64_t; using aperture_id=int32_t;
constexpr int B_OK=0,B_BUSY=-1,B_BAD_VALUE=-2,B_NO_MEMORY=-3,B_NOT_SUPPORTED=-4;
constexpr int B_MODULE_INIT=1,B_MODULE_UNINIT=2,B_KEEP_LOADED=1;
constexpr int B_ANY_KERNEL_ADDRESS=0,B_KERNEL_READ_AREA=1,B_KERNEL_WRITE_AREA=2,B_CLONEABLE_AREA=4;
constexpr int B_PAGE_SIZE=4096,PCI_address_type=6,PCI_address_type_64=4,PCI_address_space=1;
#define B_PCI_MODULE_NAME "pci"
#define B_AGP_GART_MODULE_NAME "gart"
struct module_info {const char* name;uint32 flags;status_t(*std_ops)(int32,...);};
struct aperture_info {phys_addr_t physical_base;addr_t base;size_t size,reserved_size;};
struct agp_gart_module_info {
 aperture_id(*map_aperture)(uint8,uint8,uint8,size_t,addr_t*);
 status_t(*get_aperture_info)(aperture_id,aperture_info*);
 status_t(*allocate_memory)(aperture_id,size_t,size_t,uint32,addr_t*,phys_addr_t*);
 status_t(*free_memory)(aperture_id,addr_t);
};
struct pci_info {uint8 bus,device,function,revision;uint32 vendor_id,device_id;
 struct {struct {uint32 base_registers[6],base_register_sizes[6],base_register_flags[6];} h0;} u;};
struct pci_module_info {status_t(*get_nth_pci_info)(uint32,pci_info*);};
using mutex=std::mutex;
inline void mutex_init(mutex*,const char*){}
inline void mutex_lock(mutex* m){m->lock();}
inline void mutex_unlock(mutex* m){m->unlock();}
status_t get_module(const char*,module_info**);
void put_module(const char*);
area_id map_physical_memory(const char*,phys_addr_t,size_t,uint32,uint32,void**);
'''
test = r'''
#include <cassert>
#include <cstring>
#include <new>
#include <cstdio>
#include <condition_variable>
#include <chrono>
#include <thread>
static int failure,refs,mapCalls,allocateCalls,freeCalls;
static pci_info device;
static std::mutex backendGate;
static std::condition_variable backendChanged;
static int blockedKind = -1;
static bool backendEntered, backendReleased;

static void PauseBackend(int kind)
{
 std::unique_lock<std::mutex> lock(backendGate);
 if (blockedKind != kind)
  return;
 backendEntered = true;
 backendChanged.notify_all();
 assert(backendChanged.wait_for(lock, std::chrono::seconds(5),
  [] { return backendReleased; }));
}

static status_t Enumerate(uint32 index,pci_info* out){if(index)return -5;*out=device;return B_OK;}
static aperture_id Map(uint8,uint8,uint8,size_t,addr_t* out){mapCalls++;if(failure==3)return -5;*out=0x100000;return 7;}
static status_t QueryMock(aperture_id id,aperture_info* out){assert(id==7);PauseBackend(0);if(failure==4)return -5;*out={0x80000000,0x100000,0x10000,0x1000};return B_OK;}
static status_t AllocateMock(aperture_id id,size_t,size_t,uint32,addr_t* base,phys_addr_t* physical){assert(id==7);PauseBackend(1);allocateCalls++;if(failure==6)return -5;if(base)*base=0x102000;if(physical)*physical=0x40000;return B_OK;}
static status_t FreeMock(aperture_id id,addr_t){assert(id==7);PauseBackend(2);freeCalls++;return failure==7?-5:B_OK;}
static pci_module_info pciModule{Enumerate};
static agp_gart_module_info gartModule{Map,QueryMock,AllocateMock,FreeMock};
status_t get_module(const char* name,module_info** out){
 bool pci=strcmp(name,B_PCI_MODULE_NAME)==0;
 if(failure==(pci?1:2))return -5;
 refs++;*out=reinterpret_cast<module_info*>(pci?static_cast<void*>(&pciModule):static_cast<void*>(&gartModule));return B_OK;
}
void put_module(const char*){refs--;}
area_id map_physical_memory(const char*,phys_addr_t base,size_t size,uint32,uint32 protection,void** out){
 assert(base==0x90000000&&size==0x10000);
 assert(protection==(B_KERNEL_READ_AREA|B_KERNEL_WRITE_AREA|B_CLONEABLE_AREA));
 if(failure==5){return -5;}*out=reinterpret_cast<void*>(0x200000);return 12;
}
static void Reset(int stage){
 sState.~IntelApertureLeaseState();new(&sState)IntelApertureLeaseState;
 sAperture=-1;sResources={};failure=stage;refs=mapCalls=allocateCalls=freeCalls=0;
 blockedKind=-1;backendEntered=backendReleased=false;
 device={};device.device=2;device.vendor_id=0x8086;device.device_id=0x46d1;
 device.u.h0.base_registers[0]=0x90000000;device.u.h0.base_register_sizes[0]=0x10000;
}
static uint64 Begin(){uint64 lease=0;intel_memory_owner_resources resource{};
 assert(BeginDisplay(0,2,0,&lease,&resource)==B_OK);
 assert(resource.aperture.base==0x100000&&resource.registers_area==12&&resource.registers==0x200000);
 return lease;
}
int main(){
 for(int stage=1;stage<=2;stage++){Reset(stage);assert(StandardOperations(B_MODULE_INIT)!=B_OK&&refs==0);}
 for(int stage=3;stage<=5;stage++){
  Reset(stage);assert(StandardOperations(B_MODULE_INIT)==B_OK&&refs==2);
  uint64 lease=99;intel_memory_owner_resources resource{};resource.registers=73;
  assert(BeginDisplay(0,2,0,&lease,&resource)!=B_OK&&lease==99&&resource.registers==73);
  assert(BeginDisplay(0,2,0,&lease,&resource)==B_BUSY&&mapCalls==1);
  assert(StandardOperations(B_MODULE_UNINIT)==B_BUSY&&refs==2);
 }
 for(int invalid=0;invalid<3;invalid++){
  Reset(0);assert(StandardOperations(B_MODULE_INIT)==B_OK);
  if(invalid==0)device.revision=1;
  if(invalid==1)device.u.h0.base_register_flags[0]=PCI_address_space;
  if(invalid==2)device.u.h0.base_register_sizes[0]=0;
  uint64 lease=99;intel_memory_owner_resources resource{};
  assert(BeginDisplay(0,2,0,&lease,&resource)!=B_OK&&lease==99);
 }
 for(int stage=0;stage<=2;stage++){
  Reset(0);assert(StandardOperations(B_MODULE_INIT)==B_OK);
  uint64 lease=Begin();addr_t base=0;phys_addr_t physical=0;
  assert(Allocate(lease,4096,4096,3,&base,&physical)==B_OK&&base==0x102000&&physical==0x40000);
  assert(FinishDisplay(lease,true)==B_OK&&sState.DiagnosticAllowed());
  if(stage==1){failure=6;assert(Allocate(lease,4096,4096,3,&base,&physical)!=B_OK);}
  if(stage==2){failure=7;assert(Free(lease,base)!=B_OK);}
  if(stage)assert(!sState.DiagnosticAllowed());
  failure=0;assert(Free(lease,base)==B_OK);
  assert(ReleaseDisplay(lease)==B_OK&&ReleaseDisplay(lease)==B_BAD_VALUE);
  assert(Allocate(lease,4096,4096,3,&base,&physical)==B_BAD_VALUE);
  assert(sState.BeginCleanup(true,true)==(stage?IntelApertureLeaseState::Busy:IntelApertureLeaseState::Success));
  assert(refs==2);
 }
 // Hold each actual forwarding call inside the modeled backend. Release
 // must retain its operation pin until RAII completion, including failures.
 for (int kind = 0; kind < 3; kind++) {
  for (int fail = 0; fail < 2; fail++) {
   for (int round = 0; round < 50; round++) {
    Reset(0);
    assert(StandardOperations(B_MODULE_INIT) == B_OK);
    uint64 lease = Begin();
    assert(FinishDisplay(lease, true) == B_OK);
    failure = fail ? (kind == 0 ? 4 : kind == 1 ? 6 : 7) : 0;
    blockedKind = kind;
    status_t result = B_BUSY;
    std::thread worker([&] {
     aperture_info info{};
     addr_t base = 73;
     if (kind == 0) result = Query(lease, &info);
     else if (kind == 1) result = Allocate(lease, 4096, 4096, 3, &base, nullptr);
     else result = Free(lease, 0x102000);
    });
    {
     std::unique_lock<std::mutex> lock(backendGate);
     assert(backendChanged.wait_for(lock, std::chrono::seconds(5),
      [] { return backendEntered; }));
    }
    assert(ReleaseDisplay(lease) == B_OK);
    uint64 rejected = 99;
    intel_memory_owner_resources resource{};
    assert(BeginDisplay(0, 2, 0, &rejected, &resource) == B_BUSY && rejected == 99);
    {
     std::lock_guard<std::mutex> lock(sLock);
     assert(sState.BeginCleanup(true, true) == IntelApertureLeaseState::Busy);
    }
    {
     std::lock_guard<std::mutex> lock(backendGate);
     backendReleased = true;
    }
    backendChanged.notify_all();
    worker.join();
    assert(result == (fail ? -5 : B_OK));
    assert(refs == 2 && mapCalls == 1 && sResources.registers_area == 12);
    assert(sState.BeginCleanup(true, true) == (fail && kind != 0
     ? IntelApertureLeaseState::Busy : IntelApertureLeaseState::Success));
   }
  }
 }
 // Abort an initializing display lease while each backend call is pinned.
 for (int kind = 0; kind < 3; kind++) for (int fail = 0; fail < 2; fail++) {
  Reset(0);
  assert(StandardOperations(B_MODULE_INIT) == B_OK);
  uint64 lease = Begin();
  blockedKind = kind;
  failure = fail ? (kind == 0 ? 4 : kind == 1 ? 6 : 7) : 0;
  std::thread worker([&] {
   aperture_info info{};
   addr_t base = 73;
   status_t result = kind == 0 ? Query(lease, &info)
    : kind == 1 ? Allocate(lease, 4096, 4096, 3, &base, nullptr)
    : Free(lease, 0x102000);
   assert(result == (fail ? -5 : B_OK));
  });
  {
   std::unique_lock<std::mutex> lock(backendGate);
   assert(backendChanged.wait_for(lock, std::chrono::seconds(5),
    [] { return backendEntered; }));
  }
  assert(FinishDisplay(lease, true) == B_BUSY);
  assert(FinishDisplay(lease, false) == B_BUSY);
  assert(ReleaseDisplay(lease + 1) == B_BAD_VALUE);
  assert(ReleaseDisplay(lease) == B_OK);
  assert(ReleaseDisplay(lease) == B_BAD_VALUE);
  assert(FinishDisplay(lease, true) == B_BAD_VALUE);
  aperture_info info{};
  addr_t base = 73;
  assert(Query(lease, &info) == B_BAD_VALUE);
  assert(Allocate(lease, 4096, 4096, 3, &base, nullptr) == B_BAD_VALUE);
  assert(Free(lease, 0x102000) == B_BAD_VALUE);
  {
   std::lock_guard<std::mutex> lock(sLock);
   assert(!sState.DiagnosticAllowed());
   assert(sState.BeginCleanup(true, true) == IntelApertureLeaseState::Busy);
  }
  {
   std::lock_guard<std::mutex> lock(backendGate);
   backendReleased = true;
  }
  backendChanged.notify_all();
  worker.join();
  assert(refs == 2 && sResources.registers_area == 12 && mapCalls == 1);
  assert(!sState.DiagnosticAllowed());
  assert(sState.CurrentState() == IntelApertureLeaseState::RetainedDisplay);
  assert(sState.BeginCleanup(false, true) == IntelApertureLeaseState::Busy);
  assert(sState.BeginCleanup(true, true) == (fail && kind != 0
   ? IntelApertureLeaseState::Busy : IntelApertureLeaseState::Success));
 }
 puts("300 adapter release races and initialization barriers passed; modeled backend");
 puts("Actual owner adapter initialization/forwarding/failure fixtures passed; mocked OS/GART only");
}
'''
with tempfile.TemporaryDirectory(prefix='intel-owner-adapter-') as directory:
 d=pathlib.Path(directory);(d/'kernel').mkdir();(d/'types.h').write_text(types)
 for name in ['AGP.h','module.h','KernelExport.h','PCI.h','kernel/lock.h']:(d/name).write_text('#include "types.h"\n')
 source=(pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else
         root/'src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp')
 (d/'test.cpp').write_text('#include "'+str(source)+'"\n'+test)
 subprocess.run(['g++','-std=c++17','-pthread','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(d),'-I',str(root/'headers/private/graphics/intel_extreme'),str(d/'test.cpp'),'-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,timeout=30)
