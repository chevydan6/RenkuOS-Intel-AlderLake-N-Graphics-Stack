# Offline Gen12 shader draw image

This component constructs an image on the CPU. Its native RCS integration has executed compiled shaders on the N100; see [native evidence](../../evidence/native-rcs-003/README.md). The construction fixture alone supplies no shader execution evidence. The native owner still must provide the matching PPGTT mapping, ordered submission/completion, CPU cache visibility, current sequence/CSB and idle/error checks, and sampled output proof. It must also satisfy all engine/context/topology prerequisites before submission.

`RcsDrawImage::Build(pages, variant, gpuBase)` accepts exactly eight distinct, non-null, 4KiB-aligned owned CPU pages, variant 0..3, and the fixed GPU base `0x400000`. It validates every argument before modifying memory and rejects other layouts. CPU pointer uniqueness cannot establish physical ownership; the native allocator must ensure that the eight pages and the context/ring/page tables do not alias physically. No caller command words or user GPU addresses are accepted.

| Page | GPU address | Contents |
|---|---|---|
| 0 | 0x400000 | 323-dword batch, terminating MI_BATCH_BUFFER_END |
| 1 | 0x401000 | SF/clip viewport0, CC viewport64, blend128, CC192, scissor256 |
| 2 | 0x402000 | binding table0 -> surface64; one uncompressed linear RT |
| 3 | 0x403000 | three position vec4s (48 bytes) |
| 4 | 0x404000 | exact 128-byte Mesa 26.3.0 VS |
| 5 | 0x405000 | exact 208-byte Mesa 26.3.0 FS (SIMD16 entry at +128) |
| 6 | 0x406000 | 32x32 B8G8R8A8_UNORM, pitch128, initial poison0x19537bad |
| 7 | 0x407000 | private before/after-SBA EOP scratch at offsets0/8 |

The instruction base is0x400000, dynamic base0x401000 and surface base0x402000. All references fit these eight pages; initial adapter must map these exact GPU VAs. Surface state64 and binding0 are surface-base-relative; shader KSPs0x4000/0x5000/0x5080 are instruction-base-relative. The vertex buffer address is absolute. State and code fit a single page each; no scratch, uniforms, sampling, compressed resources or aux mapping are required. Page7 EOP values are private synchronization scratch, **never completion evidence**. The ring wrapper must append an ordered, per-request breadcrumb outside this image and retain all backing until retirement is proven.

The four variants rotate which triangle half is drawn inside the4..28 viewport square. They change vertex input, while shader code remains fixed. `InteriorX/Y` and `ExteriorX/Y` give safe sample points. All variants leave (0,0) outside. A covered pixel is expected to read0xffff4000 (RGBA1,.25,0,1 encoded BGRA); untouched pixels retain the poison. Do not require exact edge pixels. There is no row padding in this32x32/pitch128 layout. Guard neighboring allocations through the native ownership/PPGTT checks.

## Rebuild and verify

From `experimental/native-gpu`:

```sh
source /home/dan/projects/native-rcs-compiler-001/final-tools/env.sh
python3 render/draw/generate.py \
 /home/dan/projects/native-rcs-compiler-263-final \
 /home/dan/projects/new-native-rcs-draw-263
cmp render/draw/RcsDrawGenerated.h /home/dan/projects/new-native-rcs-draw-263/RcsDrawGenerated.h
python3 render/draw/check_image.py /home/dan/projects/native-rcs-compiler-263-final/mesa-26.3.0
python3 render/draw/test_rejections.py /home/dan/projects/native-rcs-compiler-263-final/mesa-26.3.0
c++ -std=c++17 -Wall -Wextra -Werror -mgeneral-regs-only render/rcs_draw_smoke.cpp -o /tmp/rcs-draw-smoke
/tmp/rcs-draw-smoke
```

`generate.py` reads the verified compiler build configuration, generated `gen120_pack.h`, and compiler static libraries. It compiles the small emitter plus the upstream L3/URB helpers into a separate host workspace. It never modifies the compiler source/build/artifacts and does not maintain a Mesa fork. All Mesa dependencies and floating point are host-only; the committed generated words and integer wrapper have no Mesa runtime dependency. Regeneration must produce the identical committed header and manifest.

`check_image.py` decodes actual command words against Mesa's imported gen120 XML (independent of pack macros) and verifies real VS+FS dispatch, SIMD pointers, TRILIST, surface/resource addresses, disabled blend/depth/compression, and exact shader binary bytes. The C++ smoke verifies validation-before-write, poison, all four vertex variants, exact shader bytes and batch termination.

## Source mapping

- Mesa26.3.0 source archive SHA256`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`; generated gen120 packet packers supply every command opcode and bit position. `manifest.json` records exact emitter/packer/artifact/header identities.
- `src/gallium/drivers/iris/iris_state.c`: `iris_store_vs_state`, `iris_store_fs_state`, FS KSP mapping, `emit_urb_config`, `iris_emit_l3_config`, SF/SBE/viewport and before/after-SBA cache synchronization. `brw_compiler.h` maps SIMD8+16 to KSP0/KSP2. For this no-varying FS, `brw_compute_sbe_per_vertex_urb_read` yields read offset0/length0. No user clip distances are enabled.
- `src/intel/dev/intel_device_info.c` device0x46d1 provides max VS threads546 and PS threads/PSD64. `intel_get_urb_config` and default device L3 configuration produce VS3576 entries,64-byte allocation, starting chunk4, deref32; HS/DS/GS zero entries. These are compiled helper outputs, not invented limits. The workload assumes the admitted ADL device profile; topology admission remains the native owner's responsibility.
- IGT`lib/rendercopy_gen9.c` commit`31ec677ca24e7ed86e35f367f40a29d3d9f51c06` supplies the null-stage/constants/depth/blend setup reference and UC MOCS3 (encoded6). Its disabled-VS RECTLIST and blit shader are not used. This image executes the compiled VS and constant-output FS with TRILIST.

Intel MIT notices are retained in `LICENSE-MIT-Intel.txt`. The generated image is an offline artifact; hardware acceptance belongs to the separately recorded native submission, retirement and pixel checks.
