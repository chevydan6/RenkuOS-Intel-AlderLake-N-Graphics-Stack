#!/usr/bin/env python3
"""Generate the VM-only one-UBO lowering experiment from canonical source."""
from pathlib import Path
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: make-probe-runtime.py CANONICAL_RUNTIME OUTPUT_C")
source = Path(sys.argv[1]).read_text()
before = "static int failure(int code, char* error, size_t capacity, const char* message)\n"
callback = """static bool lower_probe_ubo(nir_builder* b,nir_instr* instr,void* data)
{
 (void)data;
 if(instr->type!=nir_instr_type_intrinsic)return false;
 nir_intrinsic_instr* in=nir_instr_as_intrinsic(instr);
 if(in->intrinsic!=nir_intrinsic_vulkan_resource_index
  && in->intrinsic!=nir_intrinsic_load_vulkan_descriptor)return false;
 b->cursor=nir_before_instr(instr);
 nir_def_replace(&in->def,nir_imm_ivec2(b,1,0));
 return true;
}

"""
guard = """ if(vs->info.num_ubos || fs->info.num_ubos){
  status=RENKU_INTEL_COMPILE_UNSUPPORTED;
  message=\"uniform buffers need native descriptor lowering\";goto done;
 }
"""
probe = """ if(vs->info.num_ubos || fs->info.num_ubos){
  nir_shader_instructions_pass(fs,lower_probe_ubo,nir_metadata_none,NULL);
  nir_lower_explicit_io(fs,nir_var_mem_ubo,nir_address_format_32bit_index_offset);
  nir_print_shader(fs,stderr);fflush(stderr);
 }
"""
assert source.count(before) == source.count(guard) == 1
Path(sys.argv[2]).write_text(source.replace(before, callback + before).replace(guard, probe))
