// SPDX-License-Identifier: MIT
// BCS copy/readback presentation bridge. Source pixels are CPU-generated;
// this application does not claim shader rendering or GPU desktop composition.
#include "BufferCopy.h"
#include "PresentationFrame.h"
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <View.h>
#include <Window.h>
#include <algorithm>
#include <atomic>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static constexpr uint32 kFrame=0x6266726d,kDone=0x62646f6e,kMode=0x626d6f64;
static constexpr uint32 kWidth=32,kHeight=32,kStride=128;
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"BGRX byte layout requires little endian");
static std::atomic<int> sExitStatus{0};
struct Options {uint32 frames=0,testFallbackAfter=0;bool cpu=false;};
static uint32 Hash(const void* data)
{const uint8* p=(const uint8*)data;uint32 h=2166136261u;for(unsigned i=0;i<4096;i++)h=(h^p[i])*16777619u;return h;}
static void MakeFrame(uint32 frame,uint32* pixels)
{
 for(unsigned y=0;y<kHeight;y++)for(unsigned x=0;x<kWidth;x++){
  uint32 red=(x*8+frame*3)&255,green=(y*8+frame*2)&255;
  bool grid=x%8==0 || y%8==0;
  pixels[y*kWidth+x]=grid?0xff203040:0xff000000|(red<<16)|(green<<8)|0x80;
 }
}
class CopyClient {
 int fFd=-1;bool fOpened=false;
 uint32 fSource=0,fDestination=0;
 NativeGpu::CompletionGate fGate;
 char fReason[96]="";
 bool Transfer(uint32 operation,uint32 handle,uint32* pixels) {
  native_gpu_buffer_io io={handle,0,4096,{}};
  if(operation==NATIVE_GPU_BUFFER_WRITE)memcpy(io.data,pixels,4096);
  if(ioctl(fFd,operation,&io,sizeof(io))!=0)return false;
  if(operation==NATIVE_GPU_BUFFER_READ)memcpy(pixels,io.data,4096);
  return true;
 }
 bool Open() {
  if(fFd>=0)return true;
  if(fOpened || !fGate.CanSubmit())return false;
  fOpened=true;fFd=open("/dev/misc/intel_native_execution",O_RDONLY);
  if(fFd<0){Disable("GPU device unavailable");return false;}
  uint32* handles[]={&fSource,&fDestination};
  for(auto handle:handles){
   native_gpu_buffer_create c={4096,0};
   if(ioctl(fFd,NATIVE_GPU_BUFFER_CREATE,&c,sizeof(c))!=0 || !c.handle){Disable("GPU buffer allocation failed");return false;}
   *handle=c.handle;
  }
  return true;
 }
public:
 ~CopyClient(){if(fFd>=0)close(fFd);}
 bool Available() const {return fGate.CanSubmit();}
 uint32 Sequence() const {return fGate.LastSequence();}
 const char* Reason() const {return fReason;}
 void Disable(const char* reason) {
  fGate.Fail();snprintf(fReason,sizeof(fReason),"%s",reason);
  if(fFd>=0){close(fFd);fFd=-1;}
  // Closing frees CPU-only session staging. The resident owner keeps all
  // GPU-visible backing on uncertainty; no fallback bitmap aliases it.
 }
 bool Copy(uint32* source,uint32* destination) {
  if(!Available() || !Open())return false;
  if(!Transfer(NATIVE_GPU_BUFFER_WRITE,fSource,source)){Disable("GPU upload failed");return false;}
  native_gpu_copy c={fSource,fDestination,kWidth,kHeight,kStride,kStride,{}};
  int status=ioctl(fFd,NATIVE_GPU_BUFFER_COPY,&c,sizeof(c));
  if(status!=0 || !fGate.Complete(c.report)){
   fprintf(stderr,"GPU stopped status=%d report=%d stage=%u sequence=%u bootstrap=%u/%u group=%u/%u\n",
    status,c.report.status,c.report.stage,c.report.sequence,c.report.bootstrap_stage,
    c.report.bootstrap_error,c.report.group_stage,c.report.group_error);
   Disable("GPU completion failed; CPU fallback");return false;
  }
  if(!Transfer(NATIVE_GPU_BUFFER_READ,fDestination,destination)
   || memcmp(source,destination,4096)!=0){Disable("GPU readback failed validation");return false;}
  return true;
 }
};
class Worker {
 Options fOptions;thread_id fThread=-1;sem_id fSlot=-1;
 BMessenger fTarget;
 std::atomic<bool> fStop{false};
 static int32 Entry(void* p){return ((Worker*)p)->Run();}
 int32 Run() {
  CopyClient client;
  uint32 source[1024],pixels[1024];
  uint32 produced=0,gpu=0,cpu=0;
  while(!fStop && (!fOptions.frames || produced<fOptions.frames)){
   status_t slot=acquire_sem_etc(fSlot,1,B_RELATIVE_TIMEOUT,100000);
   if(slot==B_TIMED_OUT || slot==B_INTERRUPTED)continue;
   if(slot!=B_OK || fStop)break;
   bigtime_t start=system_time();
   MakeFrame(produced+1,source);
   if(fOptions.testFallbackAfter && gpu>=fOptions.testFallbackAfter && client.Available())
    client.Disable("GPU disabled by client fallback check");
   bool useGpu=!forceCpu.load() && client.Available() && client.Copy(source,pixels);
   if(fStop)break;
   // CPU fallback uses independent source storage, never a pending GPU buffer.
   if(!useGpu)memcpy(pixels,source,sizeof(pixels));
   if(useGpu)gpu++;else cpu++;
   produced++;
   BMessage frame(kFrame);
   status_t added=frame.AddData("pixels",B_RAW_TYPE,pixels,sizeof(pixels));
   frame.AddInt32("frame",produced);frame.AddInt32("sequence",useGpu?client.Sequence():0);
   frame.AddBool("gpu",useGpu);frame.AddBool("available",client.Available());
   frame.AddInt64("job_us",system_time()-start);
   frame.AddString("reason",forceCpu.load()?"CPU selected":client.Reason());
   if(added!=B_OK || fTarget.SendMessage(&frame,(BHandler*)NULL,0)!=B_OK){
    release_sem(fSlot);
    if(!fStop){fprintf(stderr,"Frame handoff failed; stopping worker\n");sExitStatus=1;}
    break;
   }
   snooze(33333); // bounded demo cadence; not a performance benchmark
  }
  printf("worker stopped produced=%u gpu=%u cpu=%u last_gpu_sequence=%u gpu_available=%u\n",
   produced,gpu,cpu,client.Sequence(),client.Available()?1:0);fflush(stdout);
  if(!fStop){BMessage done(kDone);fTarget.SendMessage(&done,(BHandler*)NULL,0);}
  return B_OK;
 }
public:
 std::atomic<bool> forceCpu;
 explicit Worker(const Options& options):fOptions(options),forceCpu(options.cpu){}
 ~Worker(){Stop();if(fSlot>=0)delete_sem(fSlot);}
 bool Start(BHandler* target) {
  fTarget=BMessenger(target);fSlot=create_sem(1,"native frame handoff");
  if(fSlot<0)return false;
  fThread=spawn_thread(Entry,"native BCS window producer",B_NORMAL_PRIORITY,this);
  if(fThread<0)return false;
  if(resume_thread(fThread)!=B_OK){kill_thread(fThread);fThread=-1;return false;}
  return true;
 }
 void Consumed(){release_sem(fSlot);}
 void Stop() {
  fStop=true;
  if(fSlot>=0)release_sem(fSlot);
  if(fThread>=0){status_t result;wait_for_thread(fThread,&result);fThread=-1;}
 }
};
class FrameView:public BView {
 BBitmap fBitmap;
 bool fHasFrame=false,fGpu=false,fDone=false;
 uint32 fFrame=0,fGpuFrames=0,fCpuFrames=0,fSequence=0;
 int64 fJobUs=0;char fReason[96]="Waiting for first frame";
public:
 FrameView():BView(BRect(0,0,619,539),"completed frame",B_FOLLOW_ALL,B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE),
  fBitmap(BRect(0,0,kWidth-1,kHeight-1),B_RGB32){SetViewColor(245,245,245);}
 bool Ready() const {return fBitmap.InitCheck()==B_OK;}
 bool Receive(const BMessage& message) {
  const void* pixels=NULL;ssize_t bytes=0;int32 frame=0,sequence=0;bool gpu=false;
  if(message.FindData("pixels",B_RAW_TYPE,&pixels,&bytes)!=B_OK || bytes!=4096
   || message.FindInt32("frame",&frame)!=B_OK || uint32(frame)<=fFrame
   || message.FindInt32("sequence",&sequence)!=B_OK || message.FindBool("gpu",&gpu)!=B_OK)return false;
  NativeGpu::FrameDescription shape={kWidth,kHeight,kStride,NativeGpu::OpaqueBgrx32};
  if(!NativeGpu::CopyFrameRows(shape,pixels,size_t(bytes),fBitmap.Bits(),
   size_t(fBitmap.BitsLength()),fBitmap.BytesPerRow()))return false;
  fFrame=uint32(frame);fSequence=uint32(sequence);fGpu=gpu;fHasFrame=true;
  if(gpu)fGpuFrames++;else fCpuFrames++;
  message.FindInt64("job_us",&fJobUs);
  const char* reason="";message.FindString("reason",&reason);
  snprintf(fReason,sizeof(fReason),"%s",reason);
  printf("presented frame=%u mode=%s sequence=%u job_us=%lld bitmap_stride=%ld hash=%08x\n",
   fFrame,gpu?"BCS-copy":"CPU",fSequence,(long long)fJobUs,(long)fBitmap.BytesPerRow(),Hash(pixels));fflush(stdout);
  Invalidate();return true;
 }
 void Done(){fDone=true;printf("window frames=%u gpu=%u cpu=%u complete\n",fFrame,fGpuFrames,fCpuFrames);fflush(stdout);Invalidate();}
 void Draw(BRect) override {
  SetDrawingMode(B_OP_COPY);SetHighColor(20,20,20);
  DrawString("Native buffer presentation",BPoint(20,28));
  char text[192];
  snprintf(text,sizeof(text),"%s  |  frame %u  |  GPU %u / CPU %u%s",
   fGpu?"BCS copy + readback":"CPU fallback",fFrame,fGpuFrames,fCpuFrames,fDone?"  |  complete":"");
  DrawString(text,BPoint(20,108));
  if(!fGpu)DrawString(fReason,BPoint(20,130));
  BRect image=Bounds().InsetByCopy(20,20);image.top=150;image.bottom-=70;
  float side=std::min(image.Width(),image.Height());image.right=image.left+side;image.bottom=image.top+side;
  if(fHasFrame && side>0)DrawBitmap(&fBitmap,fBitmap.Bounds(),image);
  DrawString("32 x 32 opaque BGRX pixels; CPU-generated source, BCS copies.",BPoint(20,Bounds().bottom-43));
  DrawString("Presented by app_server. Shader renderer is not connected yet.",BPoint(20,Bounds().bottom-21));
 }
};
class PresenterWindow:public BWindow {
 Worker fWorker;FrameView* fView;BButton* fMode;
public:
 explicit PresenterWindow(const Options& options):
  BWindow(BRect(100,100,719,639),"RenkuOS native buffer presenter",B_TITLED_WINDOW,0),
  fWorker(options),fView(new FrameView),fMode(new BButton(BRect(20,54,220,82),"mode",
   options.cpu?"Use GPU copies":"Use CPU fallback",new BMessage(kMode)))
 {
  SetSizeLimits(540,1400,480,1100);AddChild(fView);fView->AddChild(fMode);
  if(!fView->Ready() || !fWorker.Start(this)){fprintf(stderr,"Cannot initialize presenter\n");sExitStatus=1;PostMessage(B_QUIT_REQUESTED);}
  Show();
 }
 ~PresenterWindow(){fWorker.Stop();printf("window destroyed; worker joined\n");fflush(stdout);}
 void MessageReceived(BMessage* message) override {
  if(message->what==kFrame){
   bool available=false;message->FindBool("available",&available);
   if(!fView->Receive(*message)){fprintf(stderr,"Invalid frame rejected\n");sExitStatus=1;}
   fMode->SetEnabled(available);
   fMode->SetLabel(!available?"GPU unavailable":fWorker.forceCpu.load()?"Use GPU copies":"Use CPU fallback");
   fWorker.Consumed();return;
  }
  if(message->what==kDone){fView->Done();fMode->SetEnabled(false);return;}
  if(message->what==kMode){
   bool cpu=!fWorker.forceCpu.load();fWorker.forceCpu=cpu;
   printf("mode selected=%s\n",cpu?"CPU":"BCS-copy");fflush(stdout);
   fMode->SetLabel(cpu?"Use GPU copies":"Use CPU fallback");return;
  }
  BWindow::MessageReceived(message);
 }
 bool QuitRequested() override {
  fWorker.Stop();printf("window closed; worker joined\n");fflush(stdout);
  be_app->PostMessage(B_QUIT_REQUESTED);return true;
 }
};
static bool Number(const char* text,uint32& value)
{char* end=NULL;errno=0;unsigned long n=strtoul(text,&end,10);if(errno || !*text || *end || n>UINT32_MAX)return false;value=uint32(n);return true;}
int main(int argc,char** argv)
{
 Options options;
 for(int i=1;i<argc;i++){
  if(strcmp(argv[i],"--cpu")==0)options.cpu=true;
  else if(strcmp(argv[i],"--frames")==0 && i+1<argc){if(!Number(argv[++i],options.frames))return 1;}
  else if(strcmp(argv[i],"--test-fallback-after")==0 && i+1<argc){if(!Number(argv[++i],options.testFallbackAfter))return 1;}
  else {fprintf(stderr,"usage: buffer_window [--cpu] [--frames N] [--test-fallback-after N]\n");return 1;}
 }
 setvbuf(stdout,NULL,_IOLBF,0);
 BApplication app("application/x-vnd.chevydan-native-buffer-presenter");
 new PresenterWindow(options);app.Run();return sExitStatus.load();
}
