#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise real framebuffer-offset and plane-format functions with register mocks."""
import pathlib,subprocess,sys,tempfile
root=pathlib.Path(sys.argv[1]);mode=(root/'mode.cpp').read_text();pipes=(root/'Pipes.cpp').read_text()
a=mode.index('static void\nset_frame_buffer_registers(');b=mode.index('\n\nvoid\nset_frame_buffer_base()',a)
c=pipes.index('void\nprogram_pipe_color_modes(');e=pipes.index('// #pragma mark - Pipe',c)
prefix=r'''
#include <cstdint>
#include <cstdio>
#include <map>
using uint32=uint32_t;
#define TRACE(...)
enum {INTEL_GROUP_96x=1,INTEL_GROUP_G4x,INTEL_GROUP_ILK,INTEL_FAMILY_SER5,INTEL_FAMILY_LAKE,INTEL_FAMILY_SOC0,INTEL_GROUP_HAS,INTEL_GROUP_VLV,INTEL_GROUP_CHV,INTEL_MODEL_ALDM,
INTEL_DISPLAY_A_OFFSET_HAS=0x701a4,INTEL_DISPLAY_A_BASE=0x70184,INTEL_DISPLAY_A_SURFACE=0x7019c,
INTEL_DISPLAY_A_CONTROL=0x70180,INTEL_DISPLAY_B_CONTROL=0x71180,
DISPLAY_CONTROL_COLOR_MASK_SKY=0x0f000000,DISPLAY_CONTROL_COLOR_MASK=0x3c000000,DISPLAY_CONTROL_GAMMA=0x40000000};
struct Device {int group=0;bool lake=true,aldm=true;bool InGroup(int v){return group==v;}bool InFamily(int v){return lake&&v==INTEL_FAMILY_LAKE;}bool IsModel(int v)const{return aldm&&v==INTEL_MODEL_ALDM;}};
struct display_mode {uint32 h_display_start=0,v_display_start=0;};
struct intel_shared_info {Device device_type;display_mode current_mode;uint32 bytes_per_row=7680,bits_per_pixel=32,frame_buffer_offset=0x100000,gpu_pci_vendor=0x8086,gpu_pci_device=0x46d1,gpu_pci_revision=0;} shared;
struct Info {intel_shared_info* shared_info;bool has_shared_layout;} info={&shared,true},*gInfo=&info;
std::map<uint32,uint32> regs;unsigned writes=0;
uint32 read32(uint32 r){return regs[r];}void write32(uint32 r,uint32 v){regs[r]=v;writes++;}
'''
suffix=r'''
int main(){unsigned errors=0,cases=0;
for(unsigned valid=0;valid<2;valid++)for(unsigned pan=0;pan<2;pan++)for(unsigned pipe=0;pipe<2;pipe++) {
 shared={};info.has_shared_layout=valid;regs.clear();writes=0;
 shared.current_mode.h_display_start=pan?7:0;shared.current_mode.v_display_start=pan?3:0;
 uint32 off=pipe*0x1000;set_frame_buffer_registers(off);
 uint32 address=(valid?INTEL_DISPLAY_A_OFFSET_HAS:INTEL_DISPLAY_A_BASE)+off;
 uint32 expected=valid?((shared.current_mode.v_display_start<<16)|shared.current_mode.h_display_start):shared.current_mode.v_display_start*7680+shared.current_mode.h_display_start*4;
 if(regs[address]!=expected||regs[INTEL_DISPLAY_A_SURFACE+off]!=0x100000||writes!=2)errors++;
 if(regs.count((valid?INTEL_DISPLAY_A_BASE:INTEL_DISPLAY_A_OFFSET_HAS)+off))errors++;
 cases++;
}
for(unsigned valid=0;valid<2;valid++)for(unsigned high=0;high<8;high++)for(unsigned extra=0;extra<2;extra++) {
 shared={};info.has_shared_layout=valid;regs.clear();writes=0;
 uint32 before=(high<<28)|0x0f000008U|(extra<<23);
 regs[INTEL_DISPLAY_A_CONTROL]=regs[INTEL_DISPLAY_B_CONTROL]=before;
 program_pipe_color_modes(0x04000000U);
 uint32 mask=valid?0x0f800000U:(DISPLAY_CONTROL_COLOR_MASK_SKY|DISPLAY_CONTROL_GAMMA);
 uint32 expected=(before&~mask)|0x04000000U;
 if(regs[INTEL_DISPLAY_A_CONTROL]!=expected||regs[INTEL_DISPLAY_B_CONTROL]!=expected||writes!=2)errors++;
 if(valid&&((regs[INTEL_DISPLAY_A_CONTROL]^before)&0x70000000U))errors++;
 cases++;
}
if(errors){std::fprintf(stderr,"%u plane cases failed\n",errors);return 1;}
std::printf("%u actual N100 plane offset, format and arbitration cases passed\n",cases);}
'''
with tempfile.TemporaryDirectory() as d:
 p=pathlib.Path(d)/'t.cpp';p.write_text(prefix+mode[a:b]+pipes[c:e]+suffix);binary=pathlib.Path(d)/'t'
 subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(p),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
