/* SPDX-License-Identifier: MIT
 * Copyright 2026 Native GPU validation contributors
 * NIR setup/compile sequence adapted from Mesa 26.3.0 blorp_brw.c:
 * Copyright 2012 Intel Corporation, MIT.
 */
#include <stdio.h>
#include <errno.h>
#include "source_identity.h"
#include <stdlib.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "compiler/nir/nir_builder.h"
#include "brw/brw_compiler.h"
#include "brw/brw_nir.h"
#include "dev/intel_device_info.h"
#include "dev/intel_debug.h"
#include "util/ralloc.h"

static void compiler_log(void *data, unsigned *id, const char *format, ...) {
   (void)data; (void)id; (void)format;
}
static void fail(const char *s) { fprintf(stderr, "%s\n", s); exit(1); }
static FILE *output(const char *dir, const char *name) {
   char path[4096];
   if (snprintf(path,sizeof(path),"%s/%s",dir,name) >= sizeof(path)) fail("output path too long");
   FILE *f=fopen(path,"wb"); if (!f) fail("cannot create output"); return f;
}
static void close_output(FILE *f) { if (fclose(f)) fail("output write failed"); }
static void binary(const char *dir,const char *name,const unsigned *code,
                   const struct brw_stage_prog_data *p) {
   if (!code || !p->program_size || p->total_scratch || p->num_relocs)
      fail("shader compilation failed or requires scratch/relocations");
   FILE *f=output(dir,name);
   if (fwrite(code,1,p->program_size,f)!=p->program_size) fail("code write failed");
   close_output(f);
}
static void stage(FILE *f,const struct brw_stage_prog_data *p) {
   fprintf(f,"\"program_size\":%u,\"total_scratch\":%u,\"total_shared\":%u,"
      "\"num_relocs\":%u,\"const_data_size\":%u,\"const_data_offset\":%u,"
      "\"dispatch_grf_start_reg\":%u,\"grf_used\":%u,\"push_sizes\":[%u,%u,%u,%u]",
      p->program_size,p->total_scratch,p->total_shared,p->num_relocs,
      p->const_data_size,p->const_data_offset,p->dispatch_grf_start_reg,p->grf_used,
      p->push_sizes[0],p->push_sizes[1],p->push_sizes[2],p->push_sizes[3]);
}
static void int_array(FILE *f,const int *p,unsigned n) {
   fputc('[',f);for(unsigned i=0;i<n;i++)fprintf(f,"%s%d",i?",":"",p[i]);fputc(']',f);
}
int main(int argc,char **argv) {
   if(argc!=2) fail("usage: native-rcs-shaders OUTPUT_DIRECTORY");
   if(mkdir(argv[1],0755)) fail("output directory must not exist and parent must be writable");
   process_intel_debug_variable();
   void *ctx=ralloc_context(NULL);
   struct intel_device_info dev;
   if(!intel_get_device_info_from_pci_id(0x46d1,&dev) || dev.ver!=12 || dev.verx10!=120)
      fail("unexpected N100 device profile");
   struct brw_compiler *c=brw_compiler_create(ctx,&dev);
   c->shader_debug_log=compiler_log; c->shader_perf_log=compiler_log;
   glsl_type_singleton_init_or_ref();
   nir_builder vs=nir_builder_init_simple_shader(MESA_SHADER_VERTEX,&c->nir_options[MESA_SHADER_VERTEX],"native_rcs_passthrough");
   nir_variable *vin=nir_variable_create(vs.shader,nir_var_shader_in,glsl_vec4_type(),"position");
   vin->data.location=VERT_ATTRIB_GENERIC0;
   nir_variable *vout=nir_variable_create(vs.shader,nir_var_shader_out,glsl_vec4_type(),"gl_Position");
   vout->data.location=VARYING_SLOT_POS;
   nir_copy_var(&vs,vout,vin);
   nir_builder fs=nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,&c->nir_options[MESA_SHADER_FRAGMENT],"native_rcs_constant");
   nir_variable *color=nir_variable_create(fs.shader,nir_var_shader_out,glsl_vec4_type(),"color");
   color->data.location=FRAG_RESULT_DATA0;
   nir_store_var(&fs,color,nir_imm_vec4(&fs,1.0f,0.25f,0.0f,1.0f),0xf);
   FILE *f=output(argv[1],"vs.nir"); nir_print_shader(vs.shader,f); close_output(f);
   f=output(argv[1],"fs.nir"); nir_print_shader(fs.shader,f); close_output(f);
   struct brw_nir_compiler_opts opts={0};
   brw_preprocess_nir(c,vs.shader,&opts);
   nir_shader_gather_info(vs.shader,nir_shader_get_entrypoint(vs.shader));
   struct brw_vs_prog_data vp={0};vp.inputs_read=vs.shader->info.inputs_read;
   brw_compute_vue_map(&dev,&vp.base.vue_map,vs.shader->info.outputs_written,vs.shader->info.separate_shader,1);
   struct brw_vs_prog_key vk={0};
   struct brw_compile_params vparams={.mem_ctx=ctx,.nir=vs.shader,.key=&vk.base,.prog_data=&vp.base.base};
   const unsigned *vcode=brw_compile(c,&vparams);
   if(!vcode)fail(vparams.error_str?vparams.error_str:"VS failed");
   brw_preprocess_nir(c,fs.shader,&opts);
   nir_shader_gather_info(fs.shader,nir_shader_get_entrypoint(fs.shader));
   struct brw_fs_prog_data fp={0};
   struct brw_fs_prog_key fk={0};fk.nr_color_regions=1;fk.multisample_fbo=INTEL_NEVER;
   struct brw_compile_params fparams={.mem_ctx=ctx,.nir=fs.shader,.key=&fk.base,.prog_data=&fp.base,.fs={.max_polygons=1}};
   const unsigned *fcode=brw_compile(c,&fparams);
   if(!fcode)fail(fparams.error_str?fparams.error_str:"FS failed");
   binary(argv[1],"vs.bin",vcode,&vp.base.base);binary(argv[1],"fs.bin",fcode,&fp.base);
   f=output(argv[1],"metadata.json");
   fprintf(f,"{\n\"schema_version\":1,\"mesa_version\":\"26.3.0\","
      "\"mesa_archive_sha256\":\"91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a\","
      "\"harness_sha256\":\"" NATIVE_RCS_SOURCE_SHA256 "\",\"pci_device_id\":18129,\"graphics_ver\":%u,\"graphics_verx10\":%u,"
      "\"position_attribute\":%u,\"constant_color\":[1,0.25,0,1],\n\"vs\":{",dev.ver,dev.verx10,VERT_ATTRIB_GENERIC0);
   stage(f,&vp.base.base);
   fprintf(f,",\"key\":{\"initialization\":\"zero\"},\"inputs_read\":\"%016"PRIx64"\","
      "\"urb_read_length\":%u,\"urb_entry_size\":%u,\"dispatch_mode\":%u,"
      "\"vue_slots_valid\":\"%016"PRIx64"\",\"vue_num_slots\":%d,\"varying_to_slot\":[",
      vp.inputs_read,vp.base.urb_read_length,vp.base.urb_entry_size,vp.base.dispatch_mode,
      vp.base.vue_map.slots_valid,vp.base.vue_map.num_slots);
   for(unsigned i=0;i<VARYING_SLOT_MAX;i++)fprintf(f,"%s%d",i?",":"",vp.base.vue_map.varying_to_slot[i]);
   fprintf(f,"],\"slot_to_varying\":[");
   for(unsigned i=0;i<vp.base.vue_map.num_slots;i++)fprintf(f,"%s%d",i?",":"",vp.base.vue_map.slot_to_varying[i]);
   fprintf(f,"],\"include_vue_handles\":%u,\"clip_distance_mask\":%u,\"cull_distance_mask\":%u,"
      "\"uses_vertexid\":%u,\"uses_instanceid\":%u,\"vf_component_packing\":[%u,%u,%u,%u]},\n\"fs\":{",
      vp.base.include_vue_handles,vp.base.clip_distance_mask,vp.base.cull_distance_mask,
      vp.uses_vertexid,vp.uses_instanceid,vp.vf_component_packing[0],vp.vf_component_packing[1],
      vp.vf_component_packing[2],vp.vf_component_packing[3]);stage(f,&fp.base);
   fprintf(f,",\"key\":{\"initialization\":\"zero\",\"nr_color_regions\":1,\"multisample_fbo\":%u},"
      "\"dispatch_8\":%u,\"dispatch_16\":%u,\"dispatch_32\":%u,"
      "\"prog_offset_16\":%u,\"prog_offset_32\":%u,"
      "\"dispatch_grf_start_reg_16\":%u,\"dispatch_grf_start_reg_32\":%u,"
      "\"num_varying_inputs\":%u,\"inputs\":\"%016"PRIx64"\","
      "\"barycentric_interp_modes\":%u,\"flat_inputs\":%u,\"urb_setup\":",
      fk.multisample_fbo,fp.dispatch_8,fp.dispatch_16,fp.dispatch_32,
      fp.prog_offset_16,fp.prog_offset_32,fp.dispatch_grf_start_reg_16,fp.dispatch_grf_start_reg_32,
      fp.num_varying_inputs,fp.inputs,fp.barycentric_interp_modes,fp.flat_inputs);
   int_array(f,fp.urb_setup,VARYING_SLOT_MAX);
   fprintf(f,",\"computed_depth_mode\":%u,\"computed_stencil\":%u,\"early_fragment_tests\":%u,"
      "\"uses_kill\":%u,\"uses_omask\":%u,\"uses_src_depth\":%u,\"uses_src_w\":%u,"
      "\"persample_dispatch\":%u,\"coarse_pixel_dispatch\":%u,\"has_side_effects\":%u}\n}\n",
      fp.computed_depth_mode,fp.computed_stencil,fp.early_fragment_tests,fp.uses_kill,fp.uses_omask,
      fp.uses_src_depth,fp.uses_src_w,fp.persample_dispatch,fp.coarse_pixel_dispatch,fp.has_side_effects);
   close_output(f);
   glsl_type_singleton_decref(); ralloc_free(ctx);return 0;
}
