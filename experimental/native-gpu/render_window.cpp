// SPDX-License-Identifier: MIT
// Native fixed RCS shader workload; CPU-visible readback presented by app_server.
// GPU mode sends no source pixels or GPU commands from userspace.
#include "render/RenderPresentation.h"
#include "PresentationFrame.h"
#include <Application.h>
#include <Bitmap.h>
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

static constexpr uint32 kFrame=0x6266726d,kDone=0x72646f6e;
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
class RenderClient {
 int fFd=-1;bool fOpened=false;
 NativeGpu::RenderCompletionGate fGate;
 native_gpu_render_command fCommand={};
 char fReason[160]="";
 bool Request(uint32 mode,uint32 variant=0) {
  memset(&fCommand,0,sizeof(fCommand));
  fCommand.version=1;fCommand.mode=mode;fCommand.variant=variant;
  int status=ioctl(fFd,NATIVE_GPU_RENDER_IOCTL,&fCommand,sizeof(fCommand));
  if(status==0)return true;
  fprintf(stderr,"RCS ioctl mode=%u return=%d errno=%d report=%d stage=%u sequence=%u pci_error=%u csb_error=%u safety=%u\n",
   mode,status,errno,fCommand.report.status,fCommand.report.stage,fCommand.report.sequence,
   fCommand.report.pci_error,fCommand.report.csb_error,fCommand.report.safety_reason);
  Disable("RCS request failed; CPU fallback");return false;
 }
 bool Open() {
  if(fFd>=0)return true;
  if(fOpened || !Available())return false;
  fOpened=true;fFd=open("/dev/misc/intel_native_execution",O_RDONLY);
  if(fFd<0){Disable("RCS device unavailable; CPU fallback");return false;}
  if(!Request(NATIVE_GPU_RENDER_PROBE))return false;
  const auto& r=fCommand.report;
  if(r.version!=1 || r.mode!=NATIVE_GPU_RENDER_PROBE || r.status || r.stage!=6
   || !r.owner_granted || !r.known_profile || r.refusal){
   Disable("RCS owner/profile probe refused; CPU fallback");return false;
  }
  if(!(r.state_flags&NATIVE_GPU_RENDER_DEFAULT_VALID)){
   if(!Request(NATIVE_GPU_RENDER_BOOTSTRAP))return false;
   if(!fGate.Complete(fCommand.report,NATIVE_GPU_RENDER_BOOTSTRAP)){
    Disable("RCS bootstrap completion rejected; CPU fallback");return false;
   }
  }
  return true;
 }
public:
 ~RenderClient(){if(fFd>=0)close(fFd);}
 bool Available() const{return fGate.CanSubmit();}
 uint32 Sequence() const{return fGate.LastSequence();}
 const char* Reason() const{return fReason;}
 void Disable(const char* reason) {
  fGate.Fail();snprintf(fReason,sizeof(fReason),"%s",reason);
  if(fFd>=0){close(fFd);fFd=-1;}
 }
 bool Draw(uint32 variant,uint32* destination) {
  if(!Available() || !Open() || !Request(NATIVE_GPU_RENDER_DRAW,variant))return false;
  if(!fGate.Draw(fCommand,variant)){
   const auto& r=fCommand.report;
   fprintf(stderr,"RCS completion rejected status=%d stage=%u sequence=%u variant=%u pixels=%u/%u hash=%08x csb=%016llx/%016llx\n",
    r.status,r.stage,r.sequence,r.variant,r.matched_pixels,r.checked_pixels,r.target_hash,
    (unsigned long long)r.activation,(unsigned long long)r.completion);
   Disable("RCS completion/output rejected; CPU fallback");return false;
  }
  memcpy(destination,fCommand.output,4096);return true;
 }
};
class Worker {
 Options fOptions;thread_id fThread=-1;sem_id fSlot=-1;
 BMessenger fTarget;
 std::atomic<bool> fStop{false};
 static int32 Entry(void* p){return ((Worker*)p)->Run();}
 int32 Run() {
  RenderClient client;
  uint32 pixels[1024];
  uint32 produced=0,gpu=0,cpu=0;
  while(!fStop && (!fOptions.frames || produced<fOptions.frames)){
   status_t slot=acquire_sem_etc(fSlot,1,B_RELATIVE_TIMEOUT,100000);
   if(slot==B_TIMED_OUT || slot==B_INTERRUPTED)continue;
   if(slot!=B_OK || fStop)break;
   bigtime_t start=system_time();
   uint32 variant=produced%4;
   if(fOptions.testFallbackAfter && gpu>=fOptions.testFallbackAfter && client.Available())
    client.Disable("Client fallback check (not a GPU fault)");
   bool useGpu=!fOptions.cpu && client.Available() && client.Draw(variant,pixels);
   if(fStop)break;
   // Build an independent CPU image only after selecting fallback. Never use failed output.
   if(!useGpu)MakeFrame(produced+1,pixels);
   if(useGpu)gpu++;else cpu++;
   produced++;
   BMessage frame(kFrame);
   status_t added=frame.AddData("pixels",B_RAW_TYPE,pixels,sizeof(pixels));
   frame.AddInt32("frame",produced);frame.AddInt32("sequence",useGpu?client.Sequence():0);
   frame.AddInt32("variant",variant);frame.AddBool("gpu",useGpu);
   frame.AddInt64("job_us",system_time()-start);
   frame.AddString("reason",fOptions.cpu?"CPU explicitly selected (no GPU access)":client.Reason());
   if(added!=B_OK || fTarget.SendMessage(&frame,(BHandler*)NULL,0)!=B_OK){
    release_sem(fSlot);
    if(!fStop){fprintf(stderr,"Frame handoff failed; stopping worker\n");sExitStatus=1;}
    break;
   }
   snooze(33333); // bounded demo cadence; not a performance benchmark
  }
  printf("worker stopped produced=%u gpu=%u cpu=%u last_gpu_sequence=%u gpu_enabled=%u\n",
   produced,gpu,cpu,client.Sequence(),!fOptions.cpu && client.Available()?1:0);fflush(stdout);
  if(!fStop){BMessage done(kDone);fTarget.SendMessage(&done,(BHandler*)NULL,0);}
  return B_OK;
 }
public:
 explicit Worker(const Options& options):fOptions(options){}
 ~Worker(){Stop();if(fSlot>=0)delete_sem(fSlot);}
 bool Start(BHandler* target) {
  fTarget=BMessenger(target);fSlot=create_sem(1,"native frame handoff");
  if(fSlot<0)return false;
  fThread=spawn_thread(Entry,"native RCS window producer",B_NORMAL_PRIORITY,this);
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
 uint32 fFrame=0,fGpuFrames=0,fCpuFrames=0,fSequence=0,fVariant=0;
 int64 fJobUs=0;char fReason[160]="Waiting for first frame";
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
  int32 variant=0;message.FindInt32("variant",&variant);fVariant=uint32(variant);
  message.FindInt64("job_us",&fJobUs);
  const char* reason="";message.FindString("reason",&reason);
  snprintf(fReason,sizeof(fReason),"%s",reason);
  printf("presented frame=%u mode=%s sequence=%u variant=%u job_us=%lld bitmap_stride=%ld snapshot_hash=%08x\n",
   fFrame,gpu?"RCS-shader":"CPU",fSequence,fVariant,(long long)fJobUs,(long)fBitmap.BytesPerRow(),Hash(pixels));fflush(stdout);
  Invalidate();return true;
 }
 void Done(){fDone=true;printf("window frames=%u gpu=%u cpu=%u complete\n",fFrame,fGpuFrames,fCpuFrames);fflush(stdout);Invalidate();}
 void Draw(BRect) override {
  SetDrawingMode(B_OP_COPY);SetHighColor(20,20,20);
  DrawString("Native RCS shader triangle",BPoint(20,28));
  char text[256];
  snprintf(text,sizeof(text),"%s | frame %u | GPU frames %u / CPU frames %u%s",
   fGpu?"RCS shader":"CPU fallback",fFrame,fGpuFrames,fCpuFrames,fDone?" | complete":"");
  DrawString(text,BPoint(20,108));
  snprintf(text,sizeof(text),"sequence %u | variant %u | %s",fSequence,fVariant,fReason);
  DrawString(text,BPoint(20,130));
  BRect image=Bounds().InsetByCopy(20,20);image.top=150;image.bottom-=70;
  float side=std::min(image.Width(),image.Height());image.right=image.left+side;image.bottom=image.top+side;
  if(fHasFrame && side>0){
   DrawBitmap(&fBitmap,fBitmap.Bounds(),image);
   Sync(); // finish app_server use before the UI can replace bitmap pixels
  }
  DrawString("32 x 32 native RCS shader draw + CPU-visible readback.",BPoint(20,Bounds().bottom-43));
  DrawString("Presented by app_server; CPU fallback is a separate test image.",BPoint(20,Bounds().bottom-21));
 }
};
class PresenterWindow:public BWindow {
 Worker fWorker;FrameView* fView;
public:
 explicit PresenterWindow(const Options& options):
  BWindow(BRect(100,100,719,639),"RenkuOS native RCS shader triangle",B_TITLED_WINDOW,0),
  fWorker(options),fView(new FrameView)
 {
  SetSizeLimits(620,1400,480,1100);AddChild(fView);
  if(!fView->Ready() || !fWorker.Start(this)){fprintf(stderr,"Cannot initialize presenter\n");sExitStatus=1;PostMessage(B_QUIT_REQUESTED);}
  Show();
 }
 ~PresenterWindow(){fWorker.Stop();printf("window destroyed; worker joined\n");fflush(stdout);}
 void MessageReceived(BMessage* message) override {
  if(message->what==kFrame){
   if(!fView->Receive(*message)){fprintf(stderr,"Invalid frame rejected\n");sExitStatus=1;}
   fWorker.Consumed();return;
  }
  if(message->what==kDone){fView->Done();return;}
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
  else {fprintf(stderr,"usage: render_window [--cpu] [--frames N] [--test-fallback-after N]\n");return 1;}
 }
 setvbuf(stdout,NULL,_IOLBF,0);
 BApplication app("application/x-vnd.chevydan-native-rcs-presenter");
 new PresenterWindow(options);app.Run();return sExitStatus.load();
}
