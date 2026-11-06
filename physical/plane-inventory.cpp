/* SPDX-License-Identifier: MIT */
// Read-only registers from the existing driver mapping; no new PCI mapping.
// The writable shared-info clone is used only for the normal accelerant lock.
#include <OS.h>
#include <intel_extreme.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct RegisterView {
 volatile uint8* base;
 size_t size;
 bool Read(uint32 offset,uint32& value) const {
  if((offset&3)||size<4||offset>size-4)return false;
  value=*(volatile uint32*)(base+offset);
  return value!=0xffffffffU;
 }
 void Print(const char* name,uint32 offset) const {
  uint32 value;
  if(Read(offset,value))printf("%s[%06lx]=%08lx\n",name,(unsigned long)offset,(unsigned long)value);
  else printf("%s[%06lx]=unavailable\n",name,(unsigned long)offset);
 }
};
static int Inventory(const intel_shared_info& info,const RegisterView& regs) {
 uint32 dc,main;
 if(!regs.Read(0x45504,dc)||!regs.Read(0x45404,main)||dc&0x2000000bU||!(main&1)) {
  fprintf(stderr,"Display core not proven ready; no plane reads\n");return 1;
 }
 printf("DC=%08lx MAIN=%08lx framebuffer-offset=%08lx stride=%lu bpp=%lu virtual=%ux%u\n",
  (unsigned long)dc,(unsigned long)main,(unsigned long)info.frame_buffer_offset,
  (unsigned long)info.bytes_per_row,(unsigned long)info.bits_per_pixel,
  (unsigned)info.current_mode.virtual_width,(unsigned)info.current_mode.virtual_height);
 printf("mode-pixel-clock=%lu kHz\n",(unsigned long)info.current_mode.timing.pixel_clock);
 regs.Print("MBUS_CTL",0x4438c);
 regs.Print("DPLL0_ENABLE",0x46010);
 regs.Print("DPLL0_CFGCR0",0x164284);regs.Print("DPLL0_CFGCR1",0x164288);regs.Print("DPLL0_SSC",0x164b10);
 const uint32 dbuf[]={0x45008,0x44fe8,0x44300,0x44304};
 for(unsigned i=0;i<4;i++)regs.Print("DBUF_CTL",dbuf[i]);
 for(unsigned pipe=0;pipe<4;pipe++) {
  uint32 need=1U|(1U<<(2*(pipe+5)));
  if(pipe!=0)need|=1U<<2;
  if((main&need)!=need){printf("pipe%u skipped: unpowered\n",pipe);continue;}
  printf("pipe%u\n",pipe);uint32 delta=pipe*0x1000;
  regs.Print("PIPECONF",0x70008+delta);
  regs.Print("UNDERRUN_DBG1",0x70064+delta);
  regs.Print("MBUS_DBOX",0x7003c+delta);
  for(unsigned scaler=0;scaler<2;scaler++) {
   uint32 control=0x68180+pipe*0x800+scaler*0x100;
   regs.Print("PS_CTRL",control);regs.Print("PS_WIN_POS",control-0x10);regs.Print("PS_WIN_SIZE",control-0xc);
  }
  regs.Print("TRANS_FUNC",0x60400+delta);
  regs.Print("DATA_M1",0x60030+delta);regs.Print("DATA_N1",0x60034+delta);
  regs.Print("LINK_M1",0x60040+delta);regs.Print("LINK_N1",0x60044+delta);
  regs.Print("HTOTAL",0x60000+delta);regs.Print("VTOTAL",0x6000c+delta);
  regs.Print("PIPESRC",0x6001c+delta);
  regs.Print("CUR_CTL",0x70080+delta);regs.Print("CUR_DDB",0x7017c+delta);
  for(unsigned level=0;level<6;level++)regs.Print("CUR_WM",0x70140+delta+level*4);
  regs.Print("CUR_SAGV",0x70158+delta);regs.Print("CUR_SAGV_TRANS",0x7015c+delta);
  for(unsigned plane=0;plane<7;plane++) {
   uint32 base=delta+plane*0x100;
   printf("pipe%u plane%u\n",pipe,plane+1);
   regs.Print("CTL",0x70180+base);regs.Print("COLOR",0x701cc+base);
   regs.Print("STRIDE",0x70188+base);regs.Print("POS",0x7018c+base);
   regs.Print("SIZE",0x70190+base);regs.Print("OFFSET",0x701a4+base);
   regs.Print("SURF",0x7019c+base);regs.Print("DDB",0x7027c+base);
   for(unsigned level=0;level<6;level++)regs.Print("WM",0x70240+base+level*4);
   regs.Print("SAGV",0x70258+base);regs.Print("SAGV_TRANS",0x7025c+base);
   regs.Print("WM_TRANS",0x70268+base);
  }
 }
 return 0;
}
int main() {
 int fd=open("/dev/graphics/intel_extreme_000200",O_RDONLY);
 if(fd<0){perror("open");return 1;}
 intel_get_shared_info_layout layout={};layout.magic=INTEL_PRIVATE_DATA_MAGIC;
 if(ioctl(fd,INTEL_GET_SHARED_INFO_LAYOUT,&layout,sizeof(layout))<0
  ||layout.version!=INTEL_SHARED_INFO_LAYOUT_VERSION||layout.shared_info_size!=sizeof(intel_shared_info)) {
  fprintf(stderr,"Driver layout mismatch\n");close(fd);return 1;
 }
 intel_get_private_data request={};request.magic=INTEL_PRIVATE_DATA_MAGIC;
 if(ioctl(fd,INTEL_GET_PRIVATE_DATA,&request,sizeof(request))<0){close(fd);return 1;}
 area_info sharedArea;
 if(get_area_info(request.shared_info_area,&sharedArea)!=B_OK||sharedArea.size<sizeof(intel_shared_info)){close(fd);return 1;}
 intel_shared_info* shared=NULL;
 area_id sharedClone=clone_area("plane-inventory shared lock",(void**)&shared,B_ANY_ADDRESS,
  B_READ_AREA|B_WRITE_AREA,request.shared_info_area);
 if(sharedClone<B_OK){close(fd);return 1;}
 if(shared->gpu_pci_vendor!=0x8086||shared->gpu_pci_device!=0x46d1||shared->gpu_pci_revision!=0
  ||!shared->device_type.IsModel(INTEL_MODEL_ALDM)) {
  fprintf(stderr,"Unsupported GPU\n");delete_area(sharedClone);close(fd);return 1;
 }
 area_info registerArea;
 if(get_area_info(shared->registers_area,&registerArea)!=B_OK||registerArea.size<0x16a000){delete_area(sharedClone);close(fd);return 1;}
 volatile uint8* base=NULL;
 area_id registerClone=clone_area("plane-inventory readonly registers",(void**)&base,
  B_ANY_ADDRESS,B_READ_AREA,shared->registers_area);
 if(registerClone<B_OK){delete_area(sharedClone);close(fd);return 1;}
 int result=1;
 {
  Autolock locker(shared->accelerant_lock);
  if(locker.IsLocked()) {RegisterView regs={base,(size_t)registerArea.size};result=Inventory(*shared,regs);}
  else fprintf(stderr,"Modesetting lock unavailable\n");
 }
 delete_area(registerClone);delete_area(sharedClone);close(fd);return result;
}
