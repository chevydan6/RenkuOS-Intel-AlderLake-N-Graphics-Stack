// SPDX-License-Identifier: MIT
#include "RcsPciAdmission.h"
#include <IntelN100PciPower.h>
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
struct Fixture {
 unsigned anchors=0,statusReads=0;
 uint32_t secondStatus=0x18,secondCommand=0x407,secondPm=0;
 bool normalized=false;
 static bool Read(void* p,uint16_t offset,uint8_t size,uint32_t& value){
  auto& f=*(Fixture*)p;
  if(offset==0)++f.anchors;
  bool second=f.anchors>=3;
  switch(offset){
   case 0:value=0x46d18086;break;case 8:value=0x03000000;break;
   case 0x0e:value=0;break;case 4:value=second?f.secondCommand:0x407;break;
   case 6:++f.statusReads;value=second?f.secondStatus:0x10;break;
   case 0x10:value=0x100004;break;case 0x14:value=0;break;
   case 0x34:value=0x50;break;case 0x50:value=1;break;case 0x52:value=0;break;
   case 0x54:value=second?f.secondPm:0;break;default:return false;
  }
  if(f.normalized)value=NativeGpu::RcsPciAdmission::Normalize(offset,size,value);
  return true;
 }
 IntelN100PciPower::Result Capture(IntelN100PciSnapshot& output){
  return IntelN100PciPower::Capture(Read,this,0x100000,4096,output);
 }
};
int main()
{
 using A=NativeGpu::RcsPciAdmission;
 IntelN100PciSnapshot output={};Fixture strict;
 // Exact native failure: all four anchor-status reads are stable per capture,
 // first complete capture0x10, second complete capture0x18 (interrupt bit only).
 assert(strict.Capture(output)==IntelN100PciPower::Unstable && !output.identity);
 assert(strict.statusReads==4 && strict.anchors==4);
 Fixture normalized;normalized.normalized=true;
 assert(normalized.Capture(output)==IntelN100PciPower::Success);
 assert(normalized.statusReads==4 && normalized.anchors==4 && output.status==0x10);
 for(unsigned bit=0;bit<16;bit++)if(bit!=3){
  Fixture changed;changed.normalized=true;changed.secondStatus=0x10^(1U<<bit);output={};
  assert(changed.Capture(output)!=IntelN100PciPower::Success && !output.identity);
 }
 for(uint32_t raw : {0xffffU,UINT32_MAX}){
  assert(A::Normalize(6,2,raw)==raw);
  Fixture ones;ones.normalized=true;ones.secondStatus=raw;output={};
  assert(ones.Capture(output)==IntelN100PciPower::ReadFailed && !output.identity);
 }
 assert(A::Normalize(6,4,0x18)==0x18 && A::Normalize(4,2,0x18)==0x18);
 Fixture command;command.normalized=true;command.secondCommand^=1;output={};
 assert(command.Capture(output)==IntelN100PciPower::Unstable && !output.identity);
 Fixture pm;pm.normalized=true;pm.secondPm=0x8000;output={};
 assert(pm.Capture(output)==IntelN100PciPower::Unstable && !output.identity);
 puts("RCS full PCI admission: PASS (only interrupt status normalized; CPU fixture)");
}
