#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise real Intel aperture creation/deletion with injected mapping faults."""
import argparse
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path, help='production intel_gart.cpp')
parser.add_argument('--emit-cpp', type=pathlib.Path)
args = parser.parse_args()
source = args.source.read_text()

def function(name):
    position = source.index('\n' + name + '(')
    start = source.rfind('\n', 0, source.rfind('\n', 0, position)) + 1
    brace = source.index('{', position)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

prefix = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;
using status_t=int;using addr_t=uintptr_t;using phys_addr_t=uint64_t;
#ifndef B_OK
#define B_OK 0
#endif
#ifndef B_ERROR
#define B_ERROR -1
#endif
#ifndef B_BAD_VALUE
#define B_BAD_VALUE -2
#endif
#ifndef B_BUSY
#define B_BUSY -3
#endif
enum {INTEL_GROUP_ALD=1,INTEL_GRAPHICS_MEMORY_CONTROL=0,MEMORY_CONTROL_ENABLED=1,
 INTEL_PAGE_TABLE_CONTROL=0,PAGE_TABLE_ENABLED=1,GTT_PAGE_SHIFT=12};
struct Pci {uint8 bus,device,function;};
struct Type {bool alder;bool InGroup(int) const{return alder;}};
struct intel_info {
 Pci bridge,display;Type* type;bool aperture_created;
 size_t aperture_size,gtt_entries,gtt_stolen_entries;phys_addr_t scratch_page,gtt_physical_base;
 uint32* registers;
};
static Type type={true};
static intel_info sInfo={{0,0,0},{0,2,0},&type,false,0,4,1,0x100000,0,NULL};
static unsigned maps,unmaps,pteWrites,cacheFlushes,legacyWrites;
static bool failMap,mapped;
static unsigned failPte;
static status_t intel_map(intel_info& info) {
 maps++;if(failMap)return B_ERROR;
 mapped=true;if(info.aperture_size==0)info.aperture_size=4*4096;return B_OK;
}
static void intel_unmap(intel_info& info) {unmaps++;mapped=false;info.aperture_size=0;}
static uint16 get_pci_config(Pci&,unsigned,unsigned) {return 0;}
static void set_pci_config(Pci&,unsigned,unsigned,uint16) {legacyWrites++;}
static void write32(uint32*,phys_addr_t) {legacyWrites++;}
static uint32 read32(uint32*) {return 0;}
static status_t set_gtt_entry(intel_info&,size_t,phys_addr_t) {
 pteWrites++;return pteWrites==failPte?B_ERROR:B_OK;
}
static void read_gtt_entry(intel_info&,size_t) {}
static void MockCacheFlush() {cacheFlushes++;}
'''
suffix = r'''
int main() {
 unsigned errors=0,cases=0;
 auto check=[&](bool good,const char* label){cases++;if(!good){fprintf(stderr,"FAIL %s\n",label);errors++;}};
 void* output=(void*)0x1234;
 check(intel_create_aperture(0,2,0,0,&output)==B_OK&&output==NULL&&maps==1
  &&mapped&&sInfo.aperture_created&&pteWrites==3&&cacheFlushes==1,"successful initialization publishes ownership");
 output=(void*)0x1234;
 check(intel_create_aperture(0,2,0,8192,&output)==B_BUSY&&output==(void*)0x1234
  &&maps==1&&pteWrites==3&&cacheFlushes==1&&sInfo.aperture_size==16384,
  "retained singleton rejects reopen before mapping, PTEs or state change");
 intel_delete_aperture(NULL);
 check(!mapped&&!sInfo.aperture_created&&unmaps==1,"matching successful delete clears ownership");
 intel_delete_aperture(NULL);
 check(unmaps==1,"repeated delete does not unmap twice");
 failMap=true;
 check(intel_create_aperture(0,2,0,0,&output)==B_ERROR&&!sInfo.aperture_created
  &&maps==2&&!mapped,"failed initial map does not publish ownership");
 failMap=false;
 check(intel_create_aperture(0,2,0,0,&output)==B_OK&&maps==3&&sInfo.aperture_created,
  "creation can retry after initial map failure");
 intel_delete_aperture(NULL);
 failPte=pteWrites+2;
 check(intel_create_aperture(0,2,0,0,&output)==B_ERROR&&!sInfo.aperture_created
  &&!mapped&&unmaps==3,"partial PTE initialization failure cleans mappings without ownership");
 failPte=0;
 check(intel_create_aperture(0,2,0,0,&output)==B_OK&&sInfo.aperture_created&&mapped,
  "creation can retry after PTE initialization failure");
 unsigned oldMaps=maps;
 check(intel_create_aperture(0,3,0,0,&output)==B_BAD_VALUE&&maps==oldMaps,
  "unrelated PCI device does not mutate singleton");
 intel_delete_aperture(NULL);
 check(!sInfo.aperture_created&&!mapped,"later cleanup permits normal recreation");
 if(errors)return 1;
 printf("%u actual Intel singleton mapping/initialization/cleanup cases passed; hardware calls mocked\n",cases);
}
'''
create = function('intel_create_aperture')
if create.count('asm("wbinvd;");') != 1:
    raise SystemExit('Expected single cache instruction to mock, refusing unknown source')
create = create.replace('asm("wbinvd;");', 'MockCacheFlush();')
delete = function('intel_delete_aperture').replace('void* aperture', 'void* /*aperture*/')
fixture = prefix + create + '\n' + delete + suffix
if args.emit_cpp:
    args.emit_cpp.write_text(fixture)
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='gpu-intel-singleton-') as directory:
    tmp = pathlib.Path(directory)
    (tmp/'test.cpp').write_text(fixture)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',
                    '-fno-exceptions','-fno-rtti','-fsanitize=address,undefined',
                    str(tmp/'test.cpp'),'-o',str(tmp/'test')],check=True)
    raise SystemExit(subprocess.run([str(tmp/'test')]).returncode)
