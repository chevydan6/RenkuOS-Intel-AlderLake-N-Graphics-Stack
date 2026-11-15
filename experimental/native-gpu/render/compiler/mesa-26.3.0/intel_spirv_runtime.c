/* SPDX-License-Identifier: MIT
 * Copyright 2026 Native GPU validation contributors
 * Compiler setup adapted from Mesa 26.3.0 blorp_brw.c:
 * Copyright 2012 Intel Corporation, MIT.
 */
#include "IntelSpirvRuntime.h"
#include <stdio.h>
#include <string.h>
#include <spirv-tools/libspirv.h>
#include "compiler/nir/nir_builder.h"
#include "compiler/spirv/nir_spirv.h"
#include "compiler/spirv/spirv_info.h"
#include "brw/brw_compiler.h"
#include "brw/brw_nir.h"
#include "dev/intel_device_info.h"
#include "dev/intel_debug.h"
#include "util/ralloc.h"

static void compiler_log(void* data, unsigned* id, const char* format, ...)
{(void)data;(void)id;(void)format;}

struct one_ubo_binding { unsigned indices, descriptors; bool invalid; };
static bool lower_one_ubo(nir_builder* b,nir_instr* instr,void* data)
{
 if(instr->type!=nir_instr_type_intrinsic)return false;
 nir_intrinsic_instr* in=nir_instr_as_intrinsic(instr);
 struct one_ubo_binding* binding=(struct one_ubo_binding*)data;
 if(in->intrinsic==nir_intrinsic_vulkan_resource_reindex){
  binding->invalid=true;return false;
 }
 if(in->intrinsic==nir_intrinsic_vulkan_resource_index){
  binding->indices++;
  if(nir_intrinsic_desc_set(in)!=0 || nir_intrinsic_binding(in)!=0
   || nir_intrinsic_desc_type(in)!=nir_descriptor_type_uniform_buffer
   || nir_intrinsic_resource_type(in)!=nir_resource_type_uniform_buffer
   || !nir_src_is_const(in->src[0]) || nir_src_as_uint(in->src[0])!=0){
   binding->invalid=true;return false;
  }
 }else if(in->intrinsic==nir_intrinsic_load_vulkan_descriptor){
  binding->descriptors++;
  if(nir_intrinsic_desc_type(in)!=nir_descriptor_type_uniform_buffer){
   binding->invalid=true;return false;
  }
 }else return false;
 b->cursor=nir_before_instr(instr);
 nir_def_replace(&in->def,nir_imm_ivec2(b,1,0));
 return true;
}

static int failure(int code, char* error, size_t capacity, const char* message)
{
 if(error && capacity)snprintf(error,capacity,"%s",message ? message : "compiler failed");
 return code;
}

static int valid_spirv(const uint32_t* words,size_t count)
{
 return words && count>=5 && count<=262144 && words[0]==0x07230203
  && words[1]==0x00010000;
}

int renku_intel_validate_vulkan10(const uint32_t* words,size_t count,
 char* error,size_t capacity)
{
 if(!valid_spirv(words,count))return failure(RENKU_INTEL_COMPILE_INVALID,
  error,capacity,"invalid Vulkan 1.0 SPIR-V header or size");
 if(error && capacity)error[0]=0;
 spv_context context=spvContextCreate(SPV_ENV_VULKAN_1_0);
 if(!context)return failure(RENKU_INTEL_COMPILE_OUT_OF_MEMORY,error,capacity,
  "SPIR-V validator allocation failed");
 spv_diagnostic diagnostic=NULL;
 spv_result_t result=spvValidateBinary(context,words,count,&diagnostic);
 if(result!=SPV_SUCCESS && error && capacity)
  snprintf(error,capacity,"SPIR-V word %zu: %s",
   diagnostic?diagnostic->position.index:0,
   diagnostic && diagnostic->error?diagnostic->error:"validation failed");
 spvDiagnosticDestroy(diagnostic);
 spvContextDestroy(context);
 if(result==SPV_SUCCESS)return RENKU_INTEL_COMPILE_OK;
 if(result==SPV_ERROR_OUT_OF_MEMORY)return RENKU_INTEL_COMPILE_OUT_OF_MEMORY;
 if(result==SPV_ERROR_INTERNAL)return RENKU_INTEL_COMPILE_FAILED;
 return RENKU_INTEL_COMPILE_INVALID;
}

static void fill_stage(RenkuIntelStage* out,const struct brw_stage_prog_data* p)
{
 out->bytes=p->program_size;out->grf_used=p->grf_used;
 out->dispatch_grf_start_reg=p->dispatch_grf_start_reg;
 out->total_scratch=p->total_scratch;out->num_relocs=p->num_relocs;
}

int renku_intel_compile_vulkan_pair(const uint32_t* vertex,size_t vertex_words,
 const uint32_t* fragment,size_t fragment_words,
 RenkuIntelCompilePair* output,char* error,size_t error_capacity)
{
 if(!output || output->abi_version!=RENKU_INTEL_COMPILER_ABI
  || !output->vertex.code || !output->fragment.code
  || !valid_spirv(vertex,vertex_words) || !valid_spirv(fragment,fragment_words))
  return failure(RENKU_INTEL_COMPILE_INVALID,error,error_capacity,
   "invalid Vulkan 1.0 SPIR-V or output");
 int validation=renku_intel_validate_vulkan10(vertex,vertex_words,error,error_capacity);
 if(validation)return validation;
 validation=renku_intel_validate_vulkan10(fragment,fragment_words,error,error_capacity);
 if(validation)return validation;
 if(error && error_capacity)error[0]=0;
 process_intel_debug_variable();
 void* ctx=ralloc_context(NULL);
 if(!ctx)return failure(RENKU_INTEL_COMPILE_OUT_OF_MEMORY,error,error_capacity,
  "compiler allocation failed");
 struct intel_device_info dev;
 if(!intel_get_device_info_from_pci_id(0x46d1,&dev)
  || dev.ver!=12 || dev.verx10!=120){
  ralloc_free(ctx);return failure(RENKU_INTEL_COMPILE_WRONG_DEVICE,error,error_capacity,
   "unexpected N100 device profile");
 }
 struct brw_compiler* compiler=brw_compiler_create(ctx,&dev);
 if(!compiler){ralloc_free(ctx);return failure(RENKU_INTEL_COMPILE_OUT_OF_MEMORY,
  error,error_capacity,"compiler allocation failed");}
 compiler->shader_debug_log=compiler_log;compiler->shader_perf_log=compiler_log;
 glsl_type_singleton_init_or_ref();
 int status=0;const char* message=NULL;
 struct spirv_capabilities caps={0};caps.Shader=true;
 struct spirv_to_nir_options spv={0};
 spv.environment=NIR_SPIRV_VULKAN;spv.capabilities=&caps;
 spv.ubo_addr_format=nir_address_format_32bit_index_offset;
 spv.ssbo_addr_format=nir_address_format_32bit_index_offset;
 spv.phys_ssbo_addr_format=nir_address_format_64bit_global;
 spv.push_const_addr_format=nir_address_format_32bit_offset;
 spv.shared_addr_format=nir_address_format_32bit_offset;
 spv.task_payload_addr_format=nir_address_format_32bit_offset;
 nir_shader* vs=spirv_to_nir(vertex,vertex_words,NULL,MESA_SHADER_VERTEX,"main",
  &spv,&compiler->nir_options[MESA_SHADER_VERTEX]);
 nir_shader* fs=spirv_to_nir(fragment,fragment_words,NULL,MESA_SHADER_FRAGMENT,"main",
  &spv,&compiler->nir_options[MESA_SHADER_FRAGMENT]);
 if(!vs || !fs){status=RENKU_INTEL_COMPILE_FAILED;message="SPIR-V translation failed";goto done;}
 if(vs->info.num_ubos || fs->info.num_ubos){
  if(vs->info.num_ubos || fs->info.num_ubos!=1){
   status=RENKU_INTEL_COMPILE_UNSUPPORTED;
   message="only one fragment uniform buffer is supported";goto done;
  }
  struct one_ubo_binding binding={0};
  nir_shader_instructions_pass(fs,lower_one_ubo,nir_metadata_none,&binding);
  if(binding.invalid || binding.indices!=1 || binding.descriptors!=1){
   status=RENKU_INTEL_COMPILE_UNSUPPORTED;
   message="only set 0 binding 0 uniform buffer is supported";goto done;
  }
  nir_lower_explicit_io(fs,nir_var_mem_ubo,
   nir_address_format_32bit_index_offset);
 }
 struct brw_nir_compiler_opts opts={0};
 brw_preprocess_nir(compiler,vs,&opts);
 nir_shader_gather_info(vs,nir_shader_get_entrypoint(vs));
 struct brw_vs_prog_data vp={0};vp.inputs_read=vs->info.inputs_read;
 brw_compute_vue_map(&dev,&vp.base.vue_map,vs->info.outputs_written,
  vs->info.separate_shader,1);
 struct brw_vs_prog_key vk={0};
 struct brw_compile_params vparams={.mem_ctx=ctx,.nir=vs,.key=&vk.base,
  .prog_data=&vp.base.base};
 const unsigned* vcode=brw_compile(compiler,&vparams);
 if(!vcode){status=RENKU_INTEL_COMPILE_FAILED;
  message=vparams.error_str?vparams.error_str:"vertex compile failed";goto done;}
 // Match ANV's Vulkan fragment path: gl_FragCoord is at the pixel center
 // for single-sample rendering, even when the hardware payload starts at
 // integer pixel coordinates.
 brw_preprocess_nir(compiler,fs,&opts);
 nir_lower_wpos_center(fs);
 nir_shader_gather_info(fs,nir_shader_get_entrypoint(fs));
 struct brw_fs_prog_data fp={0};
 struct brw_fs_prog_key fk={0};fk.nr_color_regions=1;fk.multisample_fbo=INTEL_NEVER;
 struct brw_compile_params fparams={.mem_ctx=ctx,.nir=fs,.key=&fk.base,
  .prog_data=&fp.base,.fs={.max_polygons=1}};
 const unsigned* fcode=brw_compile(compiler,&fparams);
 if(!fcode){status=RENKU_INTEL_COMPILE_FAILED;
  message=fparams.error_str?fparams.error_str:"fragment compile failed";goto done;}
 if(!vp.base.base.program_size || !fp.base.program_size
  || vp.base.base.total_scratch || fp.base.total_scratch
  || vp.base.base.num_relocs || fp.base.num_relocs){
  status=RENKU_INTEL_COMPILE_UNSUPPORTED;
  message="unsupported shader scratch or relocations";goto done;
 }
 if(vp.base.base.program_size>output->vertex.capacity
  || fp.base.program_size>output->fragment.capacity){
  status=RENKU_INTEL_COMPILE_OUTPUT_TOO_SMALL;
  message="output code buffer too small";goto done;
 }
 RenkuIntelStage v={0},f={0};
 v.code=output->vertex.code;v.capacity=output->vertex.capacity;
 f.code=output->fragment.code;f.capacity=output->fragment.capacity;
 fill_stage(&v,&vp.base.base);fill_stage(&f,&fp.base);
 v.urb_read_length=vp.base.urb_read_length;
 v.urb_entry_size=vp.base.urb_entry_size;v.dispatch_mode=vp.base.dispatch_mode;
 f.dispatch_8=fp.dispatch_8;f.dispatch_16=fp.dispatch_16;
 f.dispatch_32=fp.dispatch_32;f.prog_offset_16=fp.prog_offset_16;
 f.prog_offset_32=fp.prog_offset_32;
 f.dispatch_grf_start_reg_16=fp.dispatch_grf_start_reg_16;
 f.dispatch_grf_start_reg_32=fp.dispatch_grf_start_reg_32;
 memcpy(v.code,vcode,v.bytes);memcpy(f.code,fcode,f.bytes);
 output->vertex=v;output->fragment=f;
done:
 if(status)failure(status,error,error_capacity,message);
 glsl_type_singleton_decref();ralloc_free(ctx);return status;
}
