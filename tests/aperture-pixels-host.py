#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Validate readonly existing-area diagnostic guards with injected OS calls."""
import pathlib
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='aperture-pixels-') as directory:
    tmp = pathlib.Path(directory)
    (tmp/'OS.h').write_text('')
    (tmp/'intel_extreme.h').write_text('')
    source = r'''
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <sys/mman.h>
typedef uint32_t uint32;typedef uint8_t uint8;typedef uintptr_t addr_t;
typedef int area_id;
static const int B_OK=0,B_SYSTEM_TEAM=1,B_ANY_ADDRESS=0,B_READ_AREA=1,B_WRITE_AREA=2;
static const int INTEL_MODEL_ALDM=7;
struct Device {bool IsModel(int) const{return true;}};
struct Mode {unsigned virtual_width,virtual_height;};
struct intel_shared_info {
 unsigned gpu_pci_vendor,gpu_pci_device,gpu_pci_revision;Device device_type;
 unsigned bits_per_pixel,bytes_per_row;Mode current_mode;unsigned frame_buffer_offset;
 uint8* graphics_memory;addr_t frame_buffer;unsigned graphics_memory_size;
};
struct area_info {area_id area;void* address;size_t size;unsigned protection;char name[32];};
static std::vector<uint32> backing(1920*1080,0x00123456);
static bool wrongSize=false,deny=false,writable=false;
static unsigned clones=0,deletes=0;
int get_next_area_info(int,ssize_t* cookie,area_info* area) {
 if((*cookie)++)return -1;
 *area={10,backing.data(),backing.size()*4-(wrongSize?4096:0),3,"intel graphics aperture"};return 0;
}
area_id clone_area(const char*,void** address,int,unsigned protection,area_id id) {
 clones++;if(protection!=B_READ_AREA||id!=10||deny)return -1;*address=backing.data();return 11;
}
int get_area_info(area_id id,area_info* area) {
 if(id!=11)return -1;
 *area={11,backing.data(),backing.size()*4,unsigned(B_READ_AREA|(writable?B_WRITE_AREA:0)),"readonly"};return 0;
}
int delete_area(area_id id){if(id!=11)return -1;deletes++;return 0;}
#include "aperture-pixels.h"
int main() {
 intel_shared_info info={0x8086,0x46d1,0,{},32,7680,{1920,1080},0,
 (uint8*)backing.data(),(addr_t)backing.data(),unsigned(backing.size()*4)};
 unsigned errors=0;
 if(ReadAperturePixels(info)!=0||clones!=1||deletes!=1)errors++;
 wrongSize=true;if(ReadAperturePixels(info)==0||clones!=1)errors++;wrongSize=false;
 deny=true;if(ReadAperturePixels(info)==0||clones!=2||deletes!=1)errors++;deny=false;
 writable=true;if(ReadAperturePixels(info)==0||clones!=3||deletes!=2)errors++;writable=false;
 info.frame_buffer_offset=4096;info.frame_buffer+=4096;
 if(ReadAperturePixels(info)==0||clones!=3)errors++;
 info.frame_buffer_offset=0;info.frame_buffer-=4096;info.gpu_pci_device=0;
 if(ReadAperturePixels(info)==0||clones!=3)errors++;
 info.gpu_pci_device=0x46d1;deny=true;
 if(ReadAperturePixels(info,NULL,true)!=0||clones!=4||deletes!=2)errors++;
 {
  void* blocked=mmap(NULL,4096,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  if(blocked==MAP_FAILED)return 2;
  uint32 value=0;
  ApertureReadGuard guard;
  if(guard.Copy((const volatile uint32*)blocked,&value,1))errors++;
  if(!guard.Copy(backing.data(),&value,1)||value!=0x00123456)errors++;
  munmap(blocked,4096);
 }
 return errors?1:0;
}
'''
    (tmp/'test.cpp').write_text(source)
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I',str(tmp),'-I',str(root/'physical'),
                    str(tmp/'test.cpp'),'-o',str(tmp/'test')],check=True)
    subprocess.run([str(tmp/'test')],check=True,capture_output=True)
print('8 existing-aperture guard cases passed, including real PROT_NONE fault recovery')
