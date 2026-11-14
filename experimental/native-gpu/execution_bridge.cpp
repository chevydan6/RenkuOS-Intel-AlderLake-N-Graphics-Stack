// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <KernelExport.h>
#include <lock.h>
#include <stdlib.h>
#include <string.h>
#include "ExecutionModule.h"
#include "OwnedBuffers.h"
int32 api_version=B_CUR_DRIVER_API_VERSION;
struct Session {
 mutex lock;
 NativeGpu::OwnedBuffers buffers;
 native_gpu_buffer_io transfer;
 native_gpu_copy command;
};
static status_t Open(const char*,uint32,void** cookie)
{
 Session* s=(Session*)malloc(sizeof(Session));
 if(!s)return B_NO_MEMORY;
 memset(s,0,sizeof(*s));mutex_init(&s->lock,"native GPU buffer session");
 *cookie=s;return B_OK;
}
static status_t Close(void*){return B_OK;}
static status_t Free(void* cookie)
{
 // Free-cookie is called after the final open-file reference/IO is gone.
 // These are CPU-only staging allocations: no DMA address or resident owner
 // points here, including after failure. The GPU backing stays in the owner.
 Session* s=(Session*)cookie;
 mutex_destroy(&s->lock);free(s);return B_OK;
}
static status_t ControlLocked(Session& s,uint32 operation,void* buffer,size_t bytes)
{
 if(operation==NATIVE_GPU_BUFFER_CREATE){
  if(bytes!=sizeof(native_gpu_buffer_create))return B_BAD_VALUE;
  native_gpu_buffer_create request={};
  status_t status=user_memcpy(&request,buffer,bytes);if(status!=B_OK)return status;
  request.handle=s.buffers.Create(request.bytes);
  if(!request.handle)return B_BAD_VALUE;
  return user_memcpy(buffer,&request,bytes);
 }
 if(operation==NATIVE_GPU_BUFFER_WRITE || operation==NATIVE_GPU_BUFFER_READ){
  if(bytes!=sizeof(s.transfer))return B_BAD_VALUE;
  status_t status=user_memcpy(&s.transfer,buffer,bytes);if(status!=B_OK)return status;
  auto& io=s.transfer;
  if(!s.buffers.Range(io.handle,io.offset,io.bytes))return B_BAD_VALUE;
  uint8* cpu=(uint8*)s.buffers.buffers[io.handle-1].words+io.offset;
  if(operation==NATIVE_GPU_BUFFER_WRITE){memcpy(cpu,io.data,io.bytes);return B_OK;}
  // Zero the unused payload, never return uninitialized kernel bytes.
  memset(io.data,0,sizeof(io.data));memcpy(io.data,cpu,io.bytes);
  return user_memcpy(buffer,&io,bytes);
 }
 bool sample=operation==NATIVE_GPU_EXECUTION_IOCTL;
 if(!sample && operation!=NATIVE_GPU_BUFFER_COPY)return B_BAD_VALUE;
 if(bytes!=(sample?sizeof(native_gpu_execution_report):sizeof(s.command)))return B_BAD_VALUE;
 if(!sample){
  status_t status=user_memcpy(&s.command,buffer,bytes);if(status!=B_OK)return status;
  s.command.report={};
  if(!s.buffers.CanCopy(s.command))return B_BAD_VALUE;
 }
 native_gpu_execution_module* module=NULL;
 status_t status=get_module(NATIVE_GPU_EXECUTION_MODULE,(module_info**)&module);
 if(status!=B_OK)return status;
 native_gpu_execution_report report={};
 if(sample)status=module->run(&report);
 else {
  s.buffers.Begin(s.command);
  status=module->copy(&s.command,s.buffers.buffers[s.command.source-1].words,
   s.buffers.buffers[s.command.destination-1].words,&s.command.report);
  // B_BUSY rejects this call before any staging/publication. Other failures
  // conservatively make both buffers unavailable; resident DMA backing stays.
  s.buffers.Finish(status==B_OK || status==B_BUSY);
 }
 put_module(NATIVE_GPU_EXECUTION_MODULE);
 status_t copied=user_memcpy(buffer,sample?(void*)&report:(void*)&s.command,bytes);
 return copied==B_OK?status:copied;
}
static status_t Control(void* cookie,uint32 operation,void* buffer,size_t bytes)
{
 Session& s=*(Session*)cookie;
 // Held across the synchronous request and user-copy operations: CPU access,
 // handle creation, copy and lifetime transitions cannot race in one session.
 mutex_lock(&s.lock);
 status_t status=ControlLocked(s,operation,buffer,bytes);
 mutex_unlock(&s.lock);return status;
}
static device_hooks hooks={Open,Close,Free,Control,NULL,NULL,NULL,NULL,NULL,NULL};
extern "C" status_t init_hardware(){return B_OK;}
extern "C" status_t init_driver(){return B_OK;}
extern "C" void uninit_driver(){}
extern "C" const char** publish_devices()
{static const char* names[]={"misc/intel_native_execution",NULL};return names;}
extern "C" device_hooks* find_device(const char* name)
{return strcmp(name,"misc/intel_native_execution")==0?&hooks:NULL;}
