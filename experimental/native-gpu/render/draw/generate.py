#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Read-only use of the verified compiler Mesa build; outputs go to a separate host workspace."""
from pathlib import Path
import argparse, json, shlex, subprocess, struct, hashlib
p=argparse.ArgumentParser()
p.add_argument('compiler_workspace',type=Path)
p.add_argument('host_workspace',type=Path)
p.add_argument('--dimension',type=int,choices=(32,64),default=32)
a=p.parse_args()
here=Path(__file__).resolve().parent
root=a.compiler_workspace.resolve(); work=a.host_workspace.resolve(); work.mkdir(parents=True,exist_ok=True)
src=root/'mesa-26.3.0'; build=root/'build'; artifacts=here.parent/'compiler/mesa-26.3.0/artifacts'
m=json.loads((artifacts/'metadata.json').read_text())
assert m['mesa_version']=='26.3.0' and m['pci_device_id']==0x46d1
vs,fs=m['vs'],m['fs']
for s in [vs,fs]:
 assert s['total_scratch']==s['total_shared']==s['num_relocs']==s['const_data_size']==0
 assert s['push_sizes']==[0,0,0,0]
assert vs['urb_entry_size']==1 and vs['dispatch_mode']==3 and vs['vue_num_slots']==2
assert fs['dispatch_8']==fs['dispatch_16']==1 and fs['dispatch_32']==0 and fs['num_varying_inputs']==0
assert m['constant_color']==[1,.25,0,1]
assert vs['inputs_read']=='0000000000008000' and vs['clip_distance_mask']==vs['cull_distance_mask']==0
assert vs['include_vue_handles']==vs['uses_vertexid']==vs['uses_instanceid']==0
assert vs['slot_to_varying']==[12,0]
for key in ['barycentric_interp_modes','flat_inputs','computed_depth_mode','computed_stencil','early_fragment_tests','uses_kill','uses_omask','uses_src_depth','uses_src_w','persample_dispatch','coarse_pixel_dispatch','has_side_effects']:
 assert fs[key]==0,key
constants={'VS_URB_SIZE':vs['urb_entry_size'],'VS_READ_LENGTH':vs['urb_read_length'],'VS_GRF':vs['dispatch_grf_start_reg'],
 'FS_GRF':fs['dispatch_grf_start_reg'],'FS_GRF16':fs['dispatch_grf_start_reg_16'],'FS_OFFSET16':fs['prog_offset_16']}
(work/'metadata.h').write_text(''.join(f'#define {k} {v}\n' for k,v in constants.items()))
# Reuse the *declared* Mesa compiler command configuration; never modify source/build.
commands=json.loads((build/'compile_commands.json').read_text())
entry=next(c for c in commands if c['file'].endswith('/native_rcs_shaders.c'))
cmd=shlex.split(entry['command']); cmd=cmd[:cmd.index('-MD')]
cmd += [f'-DRENKU_RENDER_DIM={a.dimension}','-I'+str(work),
 '-I'+str(build/'src/intel/genxml'),'-I'+str(src/'src/intel/genxml')]
objects=[]
for name,path in [('emit',here/'emit.c'),('l3',src/'src/intel/common/intel_l3_config.c'),('urb',src/'src/intel/common/intel_urb_config.c')]:
 obj=work/(name+'.o');subprocess.run(cmd+['-o',str(obj),'-c',str(path)],cwd=build,check=True);objects.append(str(obj))
ninja=(build/'build.ninja').read_text();start=ninja.index('build src/intel/compiler/brw/native-rcs-shaders:');link=ninja.index(' LINK_ARGS = ',start)
args=shlex.split(ninja[link:].splitlines()[0].split(' = ',1)[1])
subprocess.run(['c++','-o',str(work/'emit')]+objects+args,cwd=build,check=True)
packed=subprocess.check_output([str(work/'emit')])
# Add the exact compiler artifact bytes, not disassembly or regenerated shaders.
shader='\nnamespace NativeGpu { namespace '+('RcsDrawData64' if a.dimension==64 else 'RcsDrawData')+' {\n'
for name in ['vs','fs']:
 b=(artifacts/(name+'.bin')).read_bytes(); assert len(b)==m[name]['program_size']
 words=struct.unpack('<'+'I'*(len(b)//4),b)
 shader+=f'static constexpr uint32_t {name.title()}[] = {{\n'+','.join(f'0x{w:08x}U' for w in words)+'\n};\n'
shader+='} }\n'
# Generated header owns shader definitions inside its include guard.
packed=packed.replace(b'#endif\n',shader.encode()+b'#endif\n')
(work/'RcsDrawGenerated.h').write_bytes(packed)
# Human review aid: command/state image bytes independent of C++ wrapper.
manifest={'mesa_version':'26.3.0','mesa_archive_sha256':m['mesa_archive_sha256'],
 'render_dimension':a.dimension,
 'igt_commit':'31ec677ca24e7ed86e35f367f40a29d3d9f51c06','metadata_sha256':hashlib.sha256((artifacts/'metadata.json').read_bytes()).hexdigest(),
 'gen120_pack_sha256':hashlib.sha256((build/'src/intel/genxml/gen120_pack.h').read_bytes()).hexdigest(),
 'emit_sha256':hashlib.sha256((here/'emit.c').read_bytes()).hexdigest(),'generated_sha256':hashlib.sha256(packed).hexdigest(),
 'vs_sha256':hashlib.sha256((artifacts/'vs.bin').read_bytes()).hexdigest(),'fs_sha256':hashlib.sha256((artifacts/'fs.bin').read_bytes()).hexdigest(),
 'constants':constants,
 'upstream_sources_sha256':{str(path.relative_to(src)):hashlib.sha256(path.read_bytes()).hexdigest() for path in [src/'src/intel/dev/intel_device_info.c',src/'src/intel/common/intel_l3_config.c',src/'src/intel/common/intel_urb_config.c',src/'src/gallium/drivers/iris/iris_state.c',src/'src/intel/compiler/brw/brw_compiler.h',src/'src/intel/compiler/brw/brw_compile_fs.cpp']},
 'device_library_sha256':hashlib.sha256((build/'src/intel/dev/libintel_dev.a').read_bytes()).hexdigest()}
(work/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(work/'RcsDrawGenerated.h')
