#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compare offline image against extracted, pinned Linux MIT routines."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--references', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=False)
refs = json.loads((args.references / 'references.json').read_text())
for name, record in refs.items():
    assert hashlib.sha256((args.references / Path(name).name).read_bytes()).hexdigest() == record['sha256'], name
# Refuse arbitrary reference code even if a substituted manifest agrees with it.
expected = {
    'intel_lrc.c': '1c6d88c0cf02125e02d4826b09d9bbd9c1acebaab6d53e74ac36462a24eac3ba',
    'intel_lrc.h': 'e71534746b6f6720c978c0096316c51873f5afc2974d94cd4979303384a6f5b3',
    'intel_lrc_reg.h': '671b2adfb2825597425eea515b892cba1365f658d552f9a90439182f0f7f21c0',
    'intel_engine_regs.h': 'e6ae20bc2946a252a948628df6a84522c19dd45d537f5efa0e5c55d08e82961a',
    'intel_gpu_commands.h': 'd8d14b3edefb858418eb52506f5f7598ee2cd6bd8affa659e43faaf11c85e77c',
    'intel_gt_regs.h': '2d131eb5839396c099e8c63b52e8f998407936d8a34df4a547080fc13f5a91b9',
    'gen8_engine_cs.c': 'd605b5e4b74d76e29be1db9fb22b2f16d52f8ed96ad2c5746f850665f10f2c72',
}
text = {}
for name, digest in expected.items():
    data = (args.references / name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == digest, name
    text[name] = data.decode()

def function(source, name):
    # These pinned definitions have no braces in string literals/comments.
    match = re.search(r'(?:static\s+)?(?:inline\s+)?(?:void|bool|int|u32|i915_reg_t)\s*\*?\s*\n?' + name + r'\(', source)
    assert match, name
    start = source.index('{', match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    # C's field name `class` is a C++ keyword. No semantic transformation.
    return source[match.start():end].replace('->class', '->engine_class') + '\n'

prefix = r'''
// Extracted Linux code: SPDX-License-Identifier: MIT
// Copyright (c) 2014 Intel Corporation; original source headers retained below.
namespace LinuxReference {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
using u8 = uint8_t; using u32 = uint32_t; using u64 = uint64_t;
using i915_reg_t = uint32_t;
static inline int64_t sign_extend64(u64 value, unsigned bit) { return int64_t(value << (63 - bit)) >> (63 - bit); }
#define BIT(n) (1U << (n))
#define BIT_ULL(n) (1ULL << (n))
#define GENMASK_ULL(h, l) ((~0ULL >> (63 - (h))) & (~0ULL << (l)))
#define REG_BIT(n) BIT(n)
#define _MMIO(n) (n)
#define _MMIO_BASE(a, b) ((a) + (b))
#define PAGE_SIZE 4096
#define I915_GTT_PAGE_SIZE 4096
#define CACHELINE_BYTES 64
#define GRAPHICS_VER(x) ((void)(x), 12)
#define GRAPHICS_VER_FULL(x) ((void)(x), 1200)
#define IP_VER(a, b) ((a) * 100 + (b))
#define GEM_BUG_ON(x) assert(!(x))
#define BUILD_BUG_ON_ZERO(x) (0 * int(sizeof(char[1 - 2 * bool(x)])))
#define IS_ALIGNED(x, a) (((x) & ((a) - 1)) == 0)
#define _MASKED_BIT_ENABLE(x) (((x) << 16) | (x))
#define _MASKED_BIT_DISABLE(x) ((x) << 16)
#define i915_mmio_reg_offset(x) (x)
#define i915_mmio_reg_valid(x) ((x) != 0)
#define INVALID_MMIO_REG 0
#define HAS_FLAT_CCS(x) false
#define IS_DG2_G11(x) false
#define NEEDS_FASTCOLOR_BLT_WABB(x) false
#define offset_in_page(x) (reinterpret_cast<uintptr_t>(x) & 4095)
#define lower_32_bits(x) uint32_t(x)
#define upper_32_bits(x) uint32_t(uint64_t(x) >> 32)
#define RENDER_CLASS 0
#define COPY_ENGINE_CLASS 3
#define COMPUTE_CLASS 5
enum { RCS0, BCS0, VCS0, VCS2, VECS0, CCS0 };
struct uncore_mock { u32 gsi_offset; };
struct gt_mock { uncore_mock* uncore; };
struct intel_engine_cs { u32 mmio_base; int engine_class, instance, id; gt_mock* gt; void* i915 = nullptr; };
struct intel_context {
    intel_engine_cs* engine;
    struct { struct { u32 last; } runtime; } stats;
    u32 state;
    u32* cpu;
    u32* lrc_reg_state;
};
static u32 i915_ggtt_offset(u32 state) { return state; }
static bool ctx_needs_runalone(const intel_context*) { return false; }
static u32 lrc_indirect_bb(const intel_context* ce) { return ce->state + 8192; }
static u32* context_wabb(const intel_context* ce, bool per) { return ce->cpu + (per ? 3072 : 2048); }
static u32* gen8_emit_pipe_control(u32*, u32, u32) { assert(false); return nullptr; }
static u32* xehp_emit_fastcolor_blt_wabb(const intel_context*, u32*) { assert(false); return nullptr; }
'''
parts = [prefix]
for name in ['intel_gpu_commands.h', 'intel_engine_regs.h', 'intel_gt_regs.h', 'intel_lrc_reg.h']:
    parts.append(re.sub(r'^#include[^\n]*\n', '', text[name], flags=re.M))
for line in text['intel_lrc.h'].splitlines():
    if re.match(r'#define (LRC_PPHWSP_PN|LRC_PPHWSP_SZ|LRC_STATE_PN|LRC_STATE_OFFSET|DG2_PREDICATE_RESULT_WA|DG2_PREDICATE_RESULT_BB)\b', line):
        parts.append(line + '\n')
lrc = text['intel_lrc.c']
for name in ['set_offsets', 'lrc_ring_mi_mode', 'lrc_ring_bb_offset', 'lrc_ring_gpr0',
             'lrc_ring_wa_bb_per_ctx', 'lrc_ring_indirect_ptr', 'lrc_ring_indirect_offset',
             'lrc_ring_indirect_offset_default', 'lrc_setup_bb_per_ctx', 'lrc_setup_indirect_ctx',
             'init_common_regs', '__reset_stop_ring', 'gen12_emit_timestamp_wa',
             'gen12_emit_restore_scratch', 'setup_predicate_disable_wa']:
    parts.append(function(lrc, name))
parts.append(re.search(r'static const u8 gen12_xcs_offsets\[\] = \{.*?\n\};', lrc, re.S).group(0))
for name in ['gen12_get_aux_inv_reg', 'gen12_needs_ccs_aux_inv', 'gen12_emit_aux_table_inv']:
    parts.append(function(text['gen8_engine_cs.c'], name))
for name in ['gen12_emit_indirect_ctx_xcs', 'xehp_emit_per_ctx_bb', 'setup_per_ctx_bb', 'setup_indirect_ctx_bb']:
    parts.append(function(lrc, name))
stores = lrc[lrc.index('regs[CTX_RING_START] ='):lrc.index('/* RPCS */')]
parts.append(r'''
static void Build(const IntelN100ContextImage::Input& in, u32* output)
{
    memset(output, 0, 16384);
    uncore_mock uncore = {0}; gt_mock gt = {&uncore};
    intel_engine_cs engine = {0x22000, COPY_ENGINE_CLASS, 0, BCS0, &gt};
    intel_context context = {&engine, {{0}}, u32(in.contextGgtt), output, output + 1024};
    u32* regs = output + 1024;
    set_offsets(regs, gen12_xcs_offsets, &engine, true);
    init_common_regs(regs, &context, &engine, true);
    regs[CTX_PDP0_UDW] = upper_32_bits(in.pml4Dma);
    regs[CTX_PDP0_LDW] = lower_32_bits(in.pml4Dma);
    __reset_stop_ring(regs, &engine);
    struct Ring { u32 vma, tail, size; } buffer = {u32(in.ringGgtt), in.tail, in.ringBytes};
    Ring* ring = &buffer;
    u32 head = 0;
''')
parts.append(stores)
parts.append(r'''
    setup_indirect_ctx_bb(&context, &engine, gen12_emit_indirect_ctx_xcs);
    setup_per_ctx_bb(&context, &engine, xehp_emit_per_ctx_bb);
}
} // namespace LinuxReference
#pragma GCC diagnostic pop
''')
oracle = args.output / 'lrc-reference.inc'
oracle.write_text('\n'.join(parts))
command = ['g++', '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
           '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
           '-Iworkstreams/gpu-next', '-I' + str(args.output),
           'tests/n100-context-image-test.cpp', '-o', str(args.output / 'context-image-test')]
result = subprocess.run(command, capture_output=True, text=True)
(args.output / 'build.log').write_text(result.stdout + result.stderr)
print(result.stdout + result.stderr, end='')
result.check_returncode()
result = subprocess.run([str(args.output / 'context-image-test')], capture_output=True, text=True,
                        env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1'))
(args.output / 'test.log').write_text(result.stdout + result.stderr)
inputs = [Path('workstreams/gpu-next/IntelN100ContextImage.h'), Path('workstreams/gpu-next/IntelN100Ppgtt.h'),
          Path('tests/n100-context-image-test.cpp'), Path(__file__), oracle]
(args.output / 'result.json').write_text(json.dumps({'command': command, 'exit': result.returncode,
    'inputs': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
    'references': expected}, indent=2) + '\n')
print(result.stdout + result.stderr, end='')
result.check_returncode()
