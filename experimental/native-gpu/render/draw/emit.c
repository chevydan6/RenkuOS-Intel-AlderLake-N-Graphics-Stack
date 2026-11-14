/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 chevydan6
 * Packet setup adapted from Mesa 26.1.8 iris_state.c (Intel, MIT) and
 * IGT rendercopy_gen9.c 31ec677ca24e7ed86e35f367f40a29d3d9f51c06.
 * This is a HOST tool. Floating point and Mesa are never linked into kernel.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "intel/dev/intel_device_info.h"
#include "intel/common/intel_l3_config.h"
#define __gen_address_type uint64_t
#define __gen_user_data void
#define __gen_combine_address(data,loc,addr,delta) ((addr)+(delta))
#include "gen120_pack.h"
#include "metadata.h"
#define GFX12_3DSTATE_VERTEX_BUFFERS_length 1
#define GFX12_3DSTATE_VERTEX_ELEMENTS_length 1
static uint32_t image[3][1024];
static unsigned cursor;
#define EMIT(N, ...) do { struct GFX12_##N s={GFX12_##N##_header, __VA_ARGS__}; \
 assert(cursor+GFX12_##N##_length<=1024); \
 GFX12_##N##_pack(NULL,image[0]+cursor,&s);cursor+=GFX12_##N##_length; } while(0)
#define STATE(N,P,O,...) do {struct GFX12_##N s={__VA_ARGS__}; GFX12_##N##_pack(NULL,image[P]+(O)/4,&s);}while(0)
static void array(const char* name,const uint32_t* p,unsigned n) {
 printf("static constexpr uint32_t %s[%u] = {\n",name,n);
 for(unsigned i=0;i<n;++i) printf("0x%08xU,%s",p[i],i%6==5?"\n":" ");
 puts("\n};");
}
int main(void) {
 struct intel_device_info dev;
 assert(intel_get_device_info_from_pci_id(0x46d1,&dev));
 assert(dev.verx10==120);
 struct intel_urb_config urb={.size={VS_URB_SIZE,1,1,1}};
 bool constrained;
 const struct intel_l3_config* l3=intel_get_default_l3_config(&dev);
 intel_get_urb_config(&dev,l3,false,false,&urb,&constrained);
 fprintf(stderr,"ADL device limits: VS=%u PS/PSD=%u URB VS entries=%u start=%u size=%u deref=%u constrained=%u\n",dev.max_vs_threads,dev.max_threads_per_psd,urb.entries[0],urb.start[0],urb.size[0],urb.deref_block_size,constrained);
 // Dynamic page: viewport 0, CC viewport 64, blend header128 + RT132, CC192, scissor256.
 STATE(SF_CLIP_VIEWPORT,1,0,.ViewportMatrixElementm00=16,.ViewportMatrixElementm11=16,
 .ViewportMatrixElementm22=1,.ViewportMatrixElementm30=16,.ViewportMatrixElementm31=16,
 .XMinClipGuardband=-1,.XMaxClipGuardband=1,.YMinClipGuardband=-1,.YMaxClipGuardband=1,
 .XMinViewPort=0,.XMaxViewPort=31,.YMinViewPort=0,.YMaxViewPort=31);
 STATE(CC_VIEWPORT,1,64,.MinimumDepth=0,.MaximumDepth=1);
 STATE(BLEND_STATE,1,128,0);
 STATE(BLEND_STATE_ENTRY,1,132,.SourceBlendFactor=BLENDFACTOR_ONE,.DestinationBlendFactor=BLENDFACTOR_ZERO,
 .SourceAlphaBlendFactor=BLENDFACTOR_ONE,.DestinationAlphaBlendFactor=BLENDFACTOR_ZERO,
 .PreBlendColorClampEnable=true,.PostBlendColorClampEnable=true);
 STATE(COLOR_CALC_STATE,1,192,0);
 STATE(SCISSOR_RECT,1,256,.ScissorRectangleXMax=31,.ScissorRectangleYMax=31);
 // Surface page: binding entry0 -> surface64. B8G8R8A8_UNORM linear, 128-byte pitch.
 image[2][0]=64;
 STATE(RENDER_SURFACE_STATE,2,64,.SurfaceType=SURFTYPE_2D,.SurfaceFormat=0xc0,.RenderCacheReadWriteMode=1,
 .SurfaceHorizontalAlignment=1,.SurfaceVerticalAlignment=1,.Width=31,.Height=31,
 .SurfacePitch=127,.MOCS=6,.SurfaceBaseAddress=0x406000,
 .ShaderChannelSelectRed=SCS_RED,.ShaderChannelSelectGreen=SCS_GREEN,
 .ShaderChannelSelectBlue=SCS_BLUE,.ShaderChannelSelectAlpha=SCS_ALPHA);
 EMIT(PIPE_CONTROL,.CommandStreamerStallEnable=true,.RenderTargetCacheFlushEnable=true,.DepthCacheFlushEnable=true,.DCFlushEnable=true,
 .PostSyncOperation=WriteImmediateData,.Address=0x407000);
 EMIT(PIPELINE_SELECT,.PipelineSelection=_3D,.MaskBits=3);
 // iris_emit_l3_config: apply the device-selected L3 allocation.
 uint32_t l3word; struct GFX12_L3ALLOC l3s={0};
 if (l3 && l3->n[INTEL_L3P_ALL]<=126) {
 l3s.URBAllocation=l3->n[INTEL_L3P_URB]; l3s.ROAllocation=l3->n[INTEL_L3P_RO];
 l3s.DCAllocation=l3->n[INTEL_L3P_DC]; l3s.AllAllocation=l3->n[INTEL_L3P_ALL];
 } else l3s.L3FullWayAllocationEnable=true;
 GFX12_L3ALLOC_pack(NULL,&l3word,&l3s);
 EMIT(MI_LOAD_REGISTER_IMM,.RegisterOffset=GFX12_L3ALLOC_num,.DataDWord=l3word);
 EMIT(STATE_SIP,);
 EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_VS,);
 EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_HS,);
 EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_DS,);
 EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_GS,);
 EMIT(3DSTATE_PUSH_CONSTANT_ALLOC_PS,);
 EMIT(PIPE_CONTROL,.CommandStreamerStallEnable=true,.RenderTargetCacheFlushEnable=true,.DepthCacheFlushEnable=true,.DCFlushEnable=true,
 .PostSyncOperation=WriteImmediateData,.Address=0x407000);
 EMIT(STATE_BASE_ADDRESS,.GeneralStateMOCS=6,.IndirectObjectMOCS=6,.BindlessSurfaceStateMOCS=6,.BindlessSamplerStateMOCS=6,.GeneralStateBaseAddressModifyEnable=true,.GeneralStateBaseAddress=0x400000,
 .SurfaceStateBaseAddressModifyEnable=true,.SurfaceStateBaseAddress=0x402000,.SurfaceStateMOCS=6,
 .DynamicStateBaseAddressModifyEnable=true,.DynamicStateBaseAddress=0x401000,.DynamicStateMOCS=6,
 .IndirectObjectBaseAddressModifyEnable=true,.InstructionBaseAddressModifyEnable=true,
 .InstructionBaseAddress=0x400000,.InstructionMOCS=6,.StatelessDataPortAccessMOCS=6,
 .GeneralStateBufferSizeModifyEnable=true,.GeneralStateBufferSize=8,
 .DynamicStateBufferSizeModifyEnable=true,.DynamicStateBufferSize=1,
 .IndirectObjectBufferSizeModifyEnable=true,.InstructionBuffersizeModifyEnable=true,.InstructionBufferSize=8);
 EMIT(PIPE_CONTROL,.CommandStreamerStallEnable=true,.StateCacheInvalidationEnable=true,
 .TextureCacheInvalidationEnable=true,.ConstantCacheInvalidationEnable=true,.InstructionCacheInvalidateEnable=true,
 .VFCacheInvalidationEnable=true,.PostSyncOperation=WriteImmediateData,.Address=0x407008);
 const struct GFX12_3DSTATE_URB_ALLOC_VS urb_header={GFX12_3DSTATE_URB_ALLOC_VS_header};
 for(unsigned i=0;i<4;++i) {
 EMIT(3DSTATE_URB_ALLOC_VS,._3DCommandSubOpcode=urb_header._3DCommandSubOpcode+i,
 .VSURBEntryAllocationSize=urb.size[i]-1,.VSURBStartingAddressSlice0=urb.start[i],
 .VSURBStartingAddressSliceN=urb.start[i],.VSNumberofURBEntriesSlice0=urb.entries[i],.VSNumberofURBEntriesSliceN=urb.entries[i]);
 }
 EMIT(3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP,.SFClipViewportPointer=0);
 EMIT(3DSTATE_VIEWPORT_STATE_POINTERS_CC,.CCViewportPointer=64);
 EMIT(3DSTATE_BLEND_STATE_POINTERS,.BlendStatePointer=128,.BlendStatePointerValid=true);
 EMIT(3DSTATE_CC_STATE_POINTERS,.ColorCalcStatePointer=192,.ColorCalcStatePointerValid=true);
 EMIT(3DSTATE_MULTISAMPLE,); EMIT(3DSTATE_SAMPLE_MASK,.SampleMask=1);
 EMIT(3DSTATE_WM_HZ_OP,);
 EMIT(3DSTATE_CONSTANT_HS,.MOCS=6); EMIT(3DSTATE_HS,);
 EMIT(3DSTATE_BINDING_TABLE_POINTERS_HS,); EMIT(3DSTATE_SAMPLER_STATE_POINTERS_HS,);
 EMIT(3DSTATE_TE,); EMIT(3DSTATE_CONSTANT_DS,.MOCS=6); EMIT(3DSTATE_DS,);
 EMIT(3DSTATE_BINDING_TABLE_POINTERS_DS,); EMIT(3DSTATE_SAMPLER_STATE_POINTERS_DS,);
 EMIT(3DSTATE_CONSTANT_GS,.MOCS=6); EMIT(3DSTATE_GS,);
 EMIT(3DSTATE_BINDING_TABLE_POINTERS_GS,); EMIT(3DSTATE_SAMPLER_STATE_POINTERS_GS,);
 EMIT(3DSTATE_CONSTANT_VS,.MOCS=6); EMIT(3DSTATE_BINDING_TABLE_POINTERS_VS,); EMIT(3DSTATE_SAMPLER_STATE_POINTERS_VS,);
 EMIT(3DSTATE_VS,.KernelStartPointer=0x4000,.Enable=true,.SIMD8DispatchEnable=true,
 .MaximumNumberofThreads=dev.max_vs_threads-1,.DispatchGRFStartRegisterForURBData=VS_GRF,
 .VertexURBEntryReadLength=VS_READ_LENGTH,.StatisticsEnable=true);
 EMIT(3DSTATE_STREAMOUT,);
 EMIT(3DSTATE_CLIP,.ClipEnable=true,.ViewportXYClipTestEnable=true,.APIMode=APIMODE_OGL,
 .MinimumPointWidth=1,.MaximumPointWidth=1,.ForceZeroRTAIndexEnable=true);
 EMIT(3DSTATE_SBE,.ForceVertexURBEntryReadOffset=true,.ForceVertexURBEntryReadLength=true,
 .VertexURBEntryReadOffset=0,.VertexURBEntryReadLength=0,.NumberofSFOutputAttributes=0,
 .AttributeSwizzleEnable=true,.AttributeActiveComponentFormat={[0 ... 31]=ACTIVE_COMPONENT_XYZW});
 EMIT(3DSTATE_SBE_SWIZ,);
 EMIT(3DSTATE_RASTER,.CullMode=CULLMODE_NONE,.FrontWinding=CounterClockwise,
 .ViewportZNearClipTestEnable=true,.ViewportZFarClipTestEnable=true);
 EMIT(3DSTATE_SF,.ViewportTransformEnable=true,.LineWidth=1,.PointWidth=1,.DerefBlockSize=urb.deref_block_size);
 EMIT(3DSTATE_WM,); EMIT(3DSTATE_CONSTANT_PS,.MOCS=6);
 EMIT(3DSTATE_PS,.KernelStartPointer0=0x5000,.KernelStartPointer2=0x5000+FS_OFFSET16,
 .BindingTableEntryCount=1,.MaximumNumberofThreadsPerPSD=dev.max_threads_per_psd-1,
 ._8PixelDispatchEnable=true,._16PixelDispatchEnable=true,
 .DispatchGRFStartRegisterForConstantSetupData0=FS_GRF,.DispatchGRFStartRegisterForConstantSetupData2=FS_GRF16);
 EMIT(3DSTATE_PS_BLEND,.HasWriteableRT=true);
 EMIT(3DSTATE_PS_EXTRA,.PixelShaderValid=true);
 EMIT(3DSTATE_BINDING_TABLE_POINTERS_PS,); EMIT(3DSTATE_SAMPLER_STATE_POINTERS_PS,);
 EMIT(3DSTATE_SCISSOR_STATE_POINTERS,.ScissorRectPointer=256);
 EMIT(3DSTATE_WM_DEPTH_STENCIL,);
 EMIT(3DSTATE_DEPTH_BUFFER,.SurfaceType=SURFTYPE_NULL,.SurfaceFormat=D32_FLOAT,.MOCS=6);
 EMIT(3DSTATE_HIER_DEPTH_BUFFER,.MOCS=6); EMIT(3DSTATE_STENCIL_BUFFER,.MOCS=6); EMIT(3DSTATE_CLEAR_PARAMS,);
 EMIT(3DSTATE_DRAWING_RECTANGLE,.ClippedDrawingRectangleXMax=31,.ClippedDrawingRectangleYMax=31);
 EMIT(3DSTATE_VERTEX_BUFFERS,.DWordLength=3);
 STATE(VERTEX_BUFFER_STATE,0,cursor*4,.BufferPitch=16,.AddressModifyEnable=true,.MOCS=6,
 .BufferStartingAddress=0x403000,.BufferSize=48); cursor+=4;
 EMIT(3DSTATE_VERTEX_ELEMENTS,.DWordLength=1);
 STATE(VERTEX_ELEMENT_STATE,0,cursor*4,.SourceElementFormat=0,.Valid=true,
 .Component0Control=VFCOMP_STORE_SRC,.Component1Control=VFCOMP_STORE_SRC,
 .Component2Control=VFCOMP_STORE_SRC,.Component3Control=VFCOMP_STORE_SRC); cursor+=2;
 EMIT(3DSTATE_VF,); EMIT(3DSTATE_VF_SGVS,); EMIT(3DSTATE_VF_INSTANCING,);
 EMIT(3DSTATE_VF_TOPOLOGY,.PrimitiveTopologyType=_3DPRIM_TRILIST);
 EMIT(3DPRIMITIVE,.PrimitiveTopologyType=_3DPRIM_TRILIST,.VertexCountPerInstance=3,.InstanceCount=1);
 EMIT(PIPE_CONTROL,.CommandStreamerStallEnable=true,.RenderTargetCacheFlushEnable=true,.DCFlushEnable=true);
 EMIT(MI_BATCH_BUFFER_END,);
 puts("// SPDX-License-Identifier: MIT\n// Copyright (C) 2016 Intel Corporation; native adaptation 2026 chevydan6.\n// License: draw/LICENSE-MIT-Intel.txt\n// Generated by draw/generate.py with Mesa 26.1.8 gen120_pack; do not edit.\n#ifndef NATIVE_RCS_DRAW_GENERATED_H\n#define NATIVE_RCS_DRAW_GENERATED_H\n#include <stdint.h>\nnamespace NativeGpu { namespace RcsDrawData {");
 array("Batch",image[0],cursor); array("Dynamic",image[1],68); array("Surface",image[2],32);
 printf("static constexpr unsigned VsThreads=%u, PsThreadsPerPsd=%u, UrbVsEntries=%u;\n",dev.max_vs_threads,dev.max_threads_per_psd,urb.entries[0]);
 puts("} }\n#endif");
}
