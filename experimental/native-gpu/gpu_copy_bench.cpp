// SPDX-License-Identifier: MIT
// End-to-end prototype measurements, not a GPU-engine throughput benchmark.
#include "BufferCopy.h"
#include <Application.h>
#include <Bitmap.h>
#include <View.h>
#include <Window.h>
#include <OS.h>
#include <algorithm>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static constexpr unsigned kGpuRepeats=20,kTrials=3;
struct Measurement {uint32_t bytes,width;double cpuNs,gpuUs,roundtripUs,firstUs;};
static Measurement sResults[4];
static uint32_t sSource[1024],sDestination[1024],sCpu[1024];
static uint32_t sLastSequence;

// Out-of-line call and memory clobber prevent eliminating/coalescing repeated
// copies even when the source is unchanged. Timed batches amortize timer cost.
__attribute__((noinline)) static void CpuCopy(void* to,const void* from,size_t bytes)
{memcpy(to,from,bytes);asm volatile("" ::: "memory");}
static double Median(double (&values)[kTrials])
{std::sort(values,values+kTrials);return values[kTrials/2];}
static bool Transfer(int fd,uint32_t operation,uint32_t handle,const uint32_t* data)
{
 native_gpu_buffer_io io={handle,0,4096,{}};
 if(operation==NATIVE_GPU_BUFFER_WRITE)memcpy(io.data,data,4096);
 if(ioctl(fd,operation,&io,sizeof(io))!=0){perror("buffer transfer");return false;}
 if(operation==NATIVE_GPU_BUFFER_READ)memcpy(sDestination,io.data,4096);
 return true;
}
static bool Copy(int fd,native_gpu_copy& copy)
{
 int status=ioctl(fd,NATIVE_GPU_BUFFER_COPY,&copy,sizeof(copy));
 const auto& r=copy.report;
 if(status!=0 || r.status!=0 || r.stage!=14 || !r.load_attempted
  || !r.sequence || r.sequence<=sLastSequence || r.observed_sequence!=r.sequence
  || r.copy_words!=copy.width*copy.height || r.copy_matched!=r.copy_words
  || r.observed_nonce!=r.nonce || r.observed_complement!=~r.nonce
  || r.error_register || r.fault_register){
  fprintf(stderr,"copy failed ioctl=%d status=%d stage=%u sequence=%u; stop\n",status,r.status,r.stage,r.sequence);
  fprintf(stderr,"bootstrap=%u/%u group=%u/%u engine=%u/%u load=%u csb_error=%u safety=%u/%u PCI=%u command=%04x/%04x status=%04x/%04x PM=%04x/%04x\n",
   r.bootstrap_stage,r.bootstrap_error,r.group_stage,r.group_error,r.engine_stage,r.engine_error,r.load_attempted,r.csb_error,
   r.safety_reason,r.safety_phase,r.pci_result,r.expected_command,r.observed_command,r.expected_status,r.observed_status,r.expected_pm,r.observed_pm);
  return false;
 }
 sLastSequence=r.sequence;return true;
}
static bool Check(uint32_t bytes)
{
 for(unsigned i=0;i<1024;i++){
  uint32_t expected=i<bytes/4?sSource[i]:0xa5a5a5a5;
  if(sDestination[i]!=expected){fprintf(stderr,"readback mismatch word%u\n",i);return false;}
 }
 return true;
}
static bool SavePpm(const char* path,const uint32_t* data)
{
 FILE* file=fopen(path,"wb");if(!file)return false;
 fprintf(file,"P6\n32 32\n255\n");
 for(unsigned i=0;i<1024;i++){
  unsigned char rgb[]={uint8_t(data[i]>>16),uint8_t(data[i]>>8),uint8_t(data[i])};
  if(fwrite(rgb,1,3,file)!=3){fclose(file);return false;}
 }
 return fclose(file)==0;
}
class CopyView:public BView {
 BBitmap fSource,fDestination;
public:
 CopyView():BView(BRect(0,0,719,449),"GPU copy result",B_FOLLOW_ALL,B_WILL_DRAW),
  fSource(BRect(0,0,31,31),B_RGB32),fDestination(BRect(0,0,31,31),B_RGB32)
 {
  SetViewColor(245,245,245);
  if(fSource.InitCheck()==B_OK && fDestination.InitCheck()==B_OK){
   memcpy(fSource.Bits(),sSource,4096);memcpy(fDestination.Bits(),sDestination,4096);
  }
 }
 void Draw(BRect) override {
  SetHighColor(20,20,20);DrawString("Native Intel N100: copy a 32 x 32 pixel tile",BPoint(20,28));
  DrawString("Source",BPoint(20,53));DrawString("GPU readback: all pixels verified",BPoint(205,53));
  if(fSource.InitCheck()==B_OK)DrawBitmap(&fSource,BRect(20,65,147,192));
  if(fDestination.InitCheck()==B_OK)DrawBitmap(&fDestination,BRect(205,65,332,192));
  DrawString("Copy timings (median of 3 batches)",BPoint(20,228));
  DrawString("Bytes      CPU memcpy ns      GPU ioctl us      GPU roundtrip us",BPoint(20,252));
  for(unsigned i=0;i<4;i++){
   char line[160];const auto& r=sResults[i];
   snprintf(line,sizeof(line),"%5u          %8.2f             %8.2f             %8.2f",r.bytes,r.cpuNs,r.gpuUs,r.roundtripUs);
   DrawString(line,BPoint(20,275+22*i));
  }
  DrawString("GPU includes staging, cache flush, submission, polling and retirement.",BPoint(20,380));
  DrawString("Roundtrip also includes upload and readback. No speedup assumed.",BPoint(20,402));
  DrawString("Current API limit: 4 KiB. VT-d translation disabled; no general IOMMU support.",BPoint(20,424));
 }
};
class CopyWindow:public BWindow {
public:
 CopyWindow():BWindow(BRect(100,100,819,549),"N100 native tile-copy benchmark",B_TITLED_WINDOW,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
 {AddChild(new CopyView);Show();}
 bool QuitRequested() override {be_app->PostMessage(B_QUIT_REQUESTED);return true;}
};
int main(int argc,char** argv)
{
 bool show=argc==2 && strcmp(argv[1],"--show")==0;
 if(argc>1 && !show){fprintf(stderr,"usage: gpu_copy_bench [--show]\n");return 1;}
 for(unsigned y=0;y<32;y++)for(unsigned x=0;x<32;x++){
  bool grid=x%8==0 || y%8==0;
  sSource[y*32+x]=grid?0xff203040:0xff000000|(x*8<<16)|(y*8<<8)|0x80;
 }
 bigtime_t setupStart=system_time();
 int fd=open("/dev/misc/intel_native_execution",O_RDONLY);if(fd<0){perror("open GPU");return 1;}
 uint32_t handles[2];
 for(auto& h:handles){native_gpu_buffer_create c={4096,0};
  if(ioctl(fd,NATIVE_GPU_BUFFER_CREATE,&c,sizeof(c))!=0){perror("create");close(fd);return 1;}h=c.handle;}
 printf("session_open_create_us=%lld\n",(long long)(system_time()-setupStart));
 printf("GPU: %u repetitions x %u trials per mode/size; full ioctl or upload+ioctl+readback.\n",kGpuRepeats,kTrials);
 puts("CPU: warm memcpy batches >=20 ms, timer amortized; GPU data verification included in ioctl.");
 puts("First calls measured separately; device may already be initialized. Sizes limited by 4 KiB API.");
 FILE* csv=fopen("benchmark.csv","w");if(!csv){close(fd);return 1;}
 fprintf(csv,"bytes,width,height,cpu_memcpy_ns,gpu_ioctl_us,gpu_roundtrip_us,first_call_us,gpu_ioctl_over_cpu\n");
 bool okay=true;
 for(unsigned size=0;size<4 && okay;size++){
  uint32_t width=4u<<size,bytes=width*width*4;
  auto& result=sResults[size];result.bytes=bytes;result.width=width;
  for(auto& word:sDestination)word=0xa5a5a5a5;
  if(!Transfer(fd,NATIVE_GPU_BUFFER_WRITE,handles[0],sSource)
   || !Transfer(fd,NATIVE_GPU_BUFFER_WRITE,handles[1],sDestination)){okay=false;break;}
  native_gpu_copy copy={handles[0],handles[1],width,width,width*4,width*4,{}};
  bigtime_t start=system_time();okay=Copy(fd,copy);result.firstUs=double(system_time()-start);
  if(!okay || !Transfer(fd,NATIVE_GPU_BUFFER_READ,handles[1],NULL) || !Check(bytes)){okay=false;break;}
  // Measure CPU first, independently of the global cache flush in GPU calls.
  // Adaptive batches keep microsecond clock quantization below the copy cost.
  double cpu[kTrials],gpu[kTrials],roundtrip[kTrials];
  for(unsigned trial=0;trial<kTrials;trial++){
   uint32_t repeats=1024;bigtime_t elapsed;
   do {
    start=system_time();for(uint32_t n=0;n<repeats;n++)CpuCopy(sCpu,sSource,bytes);
    elapsed=system_time()-start;
    if(elapsed>=20000 || repeats>=(1u<<29))break;
    repeats*=2;
   } while(true);
   cpu[trial]=double(elapsed)*1000/repeats;
   if(memcmp(sCpu,sSource,bytes)!=0){okay=false;break;}
   start=system_time();
   for(unsigned n=0;n<kGpuRepeats;n++)if(!Copy(fd,copy)){okay=false;break;}
   gpu[trial]=double(system_time()-start)/kGpuRepeats;
   if(!okay || !Transfer(fd,NATIVE_GPU_BUFFER_READ,handles[1],NULL) || !Check(bytes)){okay=false;break;}
   start=system_time();
   for(unsigned n=0;n<kGpuRepeats;n++){
    if(!Transfer(fd,NATIVE_GPU_BUFFER_WRITE,handles[0],sSource) || !Copy(fd,copy)
     || !Transfer(fd,NATIVE_GPU_BUFFER_READ,handles[1],NULL)){okay=false;break;}
   }
   roundtrip[trial]=double(system_time()-start)/kGpuRepeats;
   printf("trial bytes=%u trial=%u cpu_ns=%.3f gpu_ioctl_us=%.3f gpu_roundtrip_us=%.3f sequence=%u\n",
    bytes,trial+1,cpu[trial],gpu[trial],roundtrip[trial],sLastSequence);fflush(stdout);
   if(!okay || !Check(bytes)){okay=false;break;}
  }
  if(!okay)break;
  result.cpuNs=Median(cpu);result.gpuUs=Median(gpu);result.roundtripUs=Median(roundtrip);
  printf("bytes=%u cpu_memcpy_ns=%.3f gpu_ioctl_us=%.3f gpu_roundtrip_us=%.3f first_call_us=%.0f gpu/cpu=%.1fx\n",
   bytes,result.cpuNs,result.gpuUs,result.roundtripUs,result.firstUs,result.gpuUs*1000/result.cpuNs);
  fprintf(csv,"%u,%u,%u,%.6f,%.6f,%.6f,%.0f,%.3f\n",bytes,width,width,result.cpuNs,result.gpuUs,result.roundtripUs,result.firstUs,result.gpuUs*1000/result.cpuNs);
  fflush(stdout);fflush(csv);
 }
 fclose(csv);close(fd);
 if(!okay){puts("Benchmark stopped: GPU completion/data not proven; no further submissions.");return 1;}
 if(!SavePpm("source.ppm",sSource) || !SavePpm("gpu-copy.ppm",sDestination))return 1;
 puts("Native tile-copy demo and benchmark: PASS; CSV and both PPM images saved.");
 if(show){BApplication app("application/x-vnd.chevydan-native-copy-bench");new CopyWindow;app.Run();}
 return 0;
}
