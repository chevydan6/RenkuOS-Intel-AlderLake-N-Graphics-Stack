// SPDX-License-Identifier: MIT
#include <Drivers.h>
#include <KernelExport.h>
#include <lock.h>
#include <stdlib.h>
#include <string.h>
#include "ExecutionModule.h"
#include "RenderModule.h"
#include "ResourceModule.h"
#include "OwnedBuffers.h"
int32 api_version=B_CUR_DRIVER_API_VERSION;
struct Session {
 mutex lock;
 bool closing;
 native_gpu_execution_module* module;
 NativeGpu::OwnedBuffers buffers;
 native_gpu_buffer_io transfer;
 native_gpu_copy command;
 native_gpu_render_module* renderModule;
 native_gpu_render_command render;
 native_gpu_render_large_module* largeModule;
 native_gpu_render_large_command large;
 native_gpu_resource_module* resourceModule;
 uint64_t resourceSession;
 native_gpu_resource_command resource;
};
static status_t Open(const char*,uint32,void** cookie)
{
 Session* s=(Session*)malloc(sizeof(Session));
 if(!s)return B_NO_MEMORY;
 memset(s,0,sizeof(*s));mutex_init(&s->lock,"native GPU buffer session");
 status_t status=get_module(NATIVE_GPU_EXECUTION_MODULE,(module_info**)&s->module);
 if(status!=B_OK){mutex_destroy(&s->lock);free(s);return status;}
 status=get_module(NATIVE_GPU_RENDER_MODULE,(module_info**)&s->renderModule);
 if(status!=B_OK){put_module(NATIVE_GPU_EXECUTION_MODULE);mutex_destroy(&s->lock);free(s);return status;}
 status=get_module(NATIVE_GPU_RENDER_LARGE_MODULE,(module_info**)&s->largeModule);
 if(status!=B_OK){put_module(NATIVE_GPU_RENDER_MODULE);put_module(NATIVE_GPU_EXECUTION_MODULE);
  mutex_destroy(&s->lock);free(s);return status;}
 status=get_module(NATIVE_GPU_RESOURCE_MODULE,(module_info**)&s->resourceModule);
 if(status!=B_OK){put_module(NATIVE_GPU_RENDER_LARGE_MODULE);
  put_module(NATIVE_GPU_RENDER_MODULE);put_module(NATIVE_GPU_EXECUTION_MODULE);
  mutex_destroy(&s->lock);free(s);return status;}
 s->module->session_opened();
 *cookie=s;return B_OK;
}
static status_t Close(void* cookie)
{
 Session& s=*(Session*)cookie;
 mutex_lock(&s.lock);s.closing=true;mutex_unlock(&s.lock);return B_OK;
}
static status_t Free(void* cookie)
{
 // Free-cookie is called after the final open-file reference/IO is gone.
 // These are CPU-only staging allocations: no DMA address or resident owner
 // points here, including after failure. The GPU backing stays in the owner.
 Session* s=(Session*)cookie;
 if(s->resourceSession)s->resourceModule->close(s->resourceSession);
 s->module->session_closed();
 put_module(NATIVE_GPU_RESOURCE_MODULE);
 put_module(NATIVE_GPU_RENDER_LARGE_MODULE);
 put_module(NATIVE_GPU_RENDER_MODULE);
 put_module(NATIVE_GPU_EXECUTION_MODULE);
 mutex_destroy(&s->lock);free(s);return B_OK;
}
static status_t ControlLocked(Session& s,uint32 operation,void* buffer,size_t bytes)
{
 if(s.closing)return B_DEV_NOT_READY;
 if(operation==NATIVE_GPU_RESOURCE_IOCTL){
  if(bytes!=sizeof(s.resource))return B_BAD_VALUE;
  status_t status=user_memcpy(&s.resource,buffer,bytes);if(status!=B_OK)return status;
  auto& c=s.resource;
  if(!NativeGpuResourceValid(c))return B_BAD_VALUE;
  if(!s.resourceSession){
   status=s.resourceModule->open(&s.resourceSession);
   if(status!=B_OK)return status;
  }
  if(c.action==NATIVE_GPU_RESOURCE_CREATE)
   status=s.resourceModule->create(s.resourceSession,c.size,&c.handle,&c.gpu,&c.size);
  else if(c.action==NATIVE_GPU_RESOURCE_DESTROY)
   status=s.resourceModule->destroy(s.resourceSession,c.handle);
  else {
   if(c.action==NATIVE_GPU_RESOURCE_READ)memset(c.data,0,sizeof(c.data));
   status=s.resourceModule->transfer(s.resourceSession,c.action==NATIVE_GPU_RESOURCE_WRITE,
    c.handle,c.offset,c.data,c.bytes);
  }
  if(status!=B_OK)return status;
  if(c.action!=NATIVE_GPU_RESOURCE_READ)memset(c.data,0,sizeof(c.data));
  status_t copied=user_memcpy(buffer,&c,bytes);
  if(copied!=B_OK && c.action==NATIVE_GPU_RESOURCE_CREATE)
   s.resourceModule->destroy(s.resourceSession,c.handle);
  return copied;
 }
 if(operation==NATIVE_GPU_RENDER_IOCTL){
  if(bytes!=sizeof(s.render))return B_BAD_VALUE;
  status_t status=user_memcpy(&s.render,buffer,bytes);if(status!=B_OK)return status;
  if(s.render.version==4 || s.render.version==5 || s.render.version==6
   || s.render.version==7 || s.render.version==8
   || s.render.version==13 || s.render.version==14
   || s.render.version==15 || s.render.version==16
   || s.render.version==17 || s.render.version==18
   || s.render.version==19 || s.render.version==20
   || s.render.version==21 || s.render.version==22
   || s.render.version==23 || s.render.version==24){
   if(!s.resourceSession){
    status=s.resourceModule->open(&s.resourceSession);
    if(status!=B_OK)return status;
   }
   status=s.resourceModule->draw(s.resourceSession,&s.render);
  }else status=s.renderModule->run(&s.render);
  status_t copied=user_memcpy(buffer,&s.render,bytes);
  return copied==B_OK?status:copied;
 }
 if(operation==NATIVE_GPU_RENDER_LARGE_IOCTL){
  if(bytes!=sizeof(s.large))return B_BAD_VALUE;
  status_t status=user_memcpy(&s.large,buffer,bytes);if(status!=B_OK)return status;
  if((s.large.base.version==10 || s.large.base.version==11
    || s.large.base.version==12 || s.large.base.version==13
    || s.large.base.version==14 || s.large.base.version==15
    || s.large.base.version==16 || s.large.base.version==17
    || s.large.base.version==18 || s.large.base.version==19
    || s.large.base.version==20 || s.large.base.version==21
    || s.large.base.version==22 || s.large.base.version==23
    || s.large.base.version==24)
   && !s.resourceSession){
   status=s.resourceModule->open(&s.resourceSession);
   if(status!=B_OK)return status;
  }
  status=s.largeModule->run(s.resourceSession,&s.large);
  status_t copied=user_memcpy(buffer,&s.large,bytes);
  return copied==B_OK?status:copied;
 }
 if(operation==NATIVE_GPU_LIFECYCLE_CONTROL){
  if(bytes!=sizeof(native_gpu_lifecycle_control))return B_BAD_VALUE;
  team_info caller={};
  status_t status=get_team_info(B_CURRENT_TEAM,&caller);
  if(status!=B_OK)return status;
  if(caller.uid!=0)return B_NOT_ALLOWED;
  native_gpu_lifecycle_control control={};
  status=user_memcpy(&control,buffer,bytes);if(status!=B_OK)return status;
  if(control.version!=1)return B_BAD_VALUE;
  return s.module->transition(control.action);
 }
 if(operation==NATIVE_GPU_LIFECYCLE_STATUS){
  if(bytes!=sizeof(native_gpu_lifecycle_report))return B_BAD_VALUE;
  native_gpu_lifecycle_report report={};
  status_t status=s.module->status(&report);
  return status==B_OK?user_memcpy(buffer,&report,bytes):status;
 }
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
 native_gpu_execution_module* module=s.module;
 status_t status;
 native_gpu_execution_report report={};
 if(sample)status=module->run(&report);
 else {
  s.buffers.Begin(s.command);
  status=module->copy(&s.command,s.buffers.buffers[s.command.source-1].words,
   s.buffers.buffers[s.command.destination-1].words,&s.command.report);
  // Stage0 is a pre-admission rejection (including quiesce/lost/interrupt).
  // B_BUSY also covers a prepublication diagnostic-owner conflict. Neither
  // invalidates CPU staging. A failed accepted request stays unavailable even
  // if device recovery later permits a different client to submit.
  s.buffers.Finish(status==B_OK,s.command.report.stage!=0 && status!=B_BUSY);
 }
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
