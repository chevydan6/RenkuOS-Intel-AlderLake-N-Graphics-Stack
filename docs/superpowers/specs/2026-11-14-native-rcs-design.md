# Native N100 render engine, followed by Mesa integration

## Intent and scope

The user selected the GPU render engine first, with the Mesa backend as the
follow-on after the render engine works. Preserve that order: establish
native Intel RCS execution and real shader drawing, then connect Iris to those
services. A renderer string, CPU llvmpipe output, RCS no-op or BCS copy does not
count as GPU rendering. No broad OpenGL compatibility claim is authorized by
the first draw.

The present native branch is `65d2c13`. Its BCS copy, lifetime and presentation
paths work on the physical N100; it has no RCS backend. The completed Mesa
26.1.8 EGL/Piglit/CTS integration is a separate **CPU llvmpipe** VM at
`/home/dan/vms/renkuos-mesa25`. The physical machine currently has Mesa22.0.5,
mesa_swpipe and the Software Pipe add-on. Neither establishes hardware OpenGL.

## Constraints

- Intel N100, PCI8086:46d1 revision0; graphics version12.0, not12.5.
- Retain Intel hardware sequences and licenses. Native adaptation remains the
  strategy; do not resume full LinuxKPI/i915 source closure.
- Change the GPU owner/bridge and userspace tools. The user explicitly excluded
  RenkuOS kernel/ACPI work. Preserve display programming, accelerant and boot media;
  a narrow private graphics-driver handoff is part of the approved coordination.
- Keep one request in flight initially and bounded polling. Admit direct DMA
  only while all observed VT-d units have translation disabled; general IOMMU
  support remains separate.
- Focused construction/build checks and real native execution, not an exhaustive
  test campaign. Hardware changes/tests require the existing exclusive lease.

## Approaches

1. **Native RCS with Intel/Mesa-generated code and state, then an Iris backend.**
   Recommended and consistent with the user-selected render-engine-first order.
   Establishes the device contracts Iris actually needs and keeps the compiler,
   layout and hardware-state logic upstream-derived.
2. **Start Iris before RCS works.** Rejected for this phase: its startup requires
   buffers, contexts, queries, submission and synchronization that the current
   two-buffer BCS API cannot supply. It would obscure engine failures behind a
   large integration surface.
3. **Handwrite a minimal shader ISA/renderer or expose arbitrary raw batches.**
   Rejected: unnecessary divergence from Intel logic or an unjustified change
   to the current owned, kernel-built command boundary.

The user chose the first order. Initial shader language, compiler packaging,
32-bit linear target size and the narrow test interface below are implementation
proposals, rather than additional user requirements.

## Phase A: native render-engine foundation

### Ownership and backing

Add RCS-specific resident state with a distinct context/ring/HWSP/result pool.
Never overwrite BCS backing to give it a second engine identity. Extend native
allocation within the existing GART owner/lease: GART remains the sole GGTT
writer, while the existing native page backend supplies PPGTT pages.

Before resetting RCS, inspect the pinned accelerant/kernel driver and actual
engine state to establish that no display/client command producer owns it.
Idle alone is not exclusive ownership. Do not introduce a competing ring writer.
Establish exclusion through the permitted GPU owner/bridge before reset or
publication. If that cannot exclude the existing accelerant's ring producer,
identify the exact handoff change needed rather than silently changing display
components or treating a read-only inventory as ownership transfer. Preserve the
known-working display baseline during this investigation.

The matching native profile already guards ALD ring allocation, setup, teardown
and legacy producer hooks. The remaining missing contract is a trusted private
acknowledgment from that graphics driver to the resident owner. Add only a call
after successful display-owner initialization, passing its real display lease
and the supported display-only profile. Old drivers never acknowledge and cannot
admit RCS. A missing acknowledgment leaves normal display usable. Withdraw/drain
inside the resident owner's display-release path. Do not expose a user ioctl
that can assert this profile, or change display registers to implement it.

Classify each initialization register change as engine-local or shared GT state.
Shared PAT/MOCS/workarounds/MCR changes require device-wide serialization through
the existing owner and compatible configuration; RCS exclusion alone does not
grant ownership over another engine's state. Do not restore shared registers
independently of other users. Resolve any missing ownership contract before
hardware publication, while continuing offline shader/context work independently.

The initial target is an owned, uncompressed linear32-bit image. Choose a small
surface compatible with the first bounded resource layout; size is an engineering
choice, not a guarantee that existing4KiB data buffers are sufficient. Account for
shader code, vertex/state heaps, batch, render target and readback separately.
Add narrowly required backing capacity instead of silently aliasing resources.

### RCS context and initialization

Use Linux v6.6's N100/ADL-P predicates and exact `gen12_rcs_offsets`, not the BCS
table with its base changed. Hardware context size is14 pages including ppHWSP;
the indirect workaround page makes15. A separate native terminating per-context
batch, if retained, makes16 pages/64KiB. Respect all context value slots, NOP gaps,
LRI flags, register repetition, topology-derived RPCS and context isolation.

Preserve RCS indirect timestamp/CMD_BUF_CCTL/GPR0 restore, RCS AUX invalidation
at0x4208, relevant engine/general/context workarounds, MCR steering/broadcast,
MOCS/PAT and render/GT forcewake requirements. RCS reset domain is bit1, not BCS
bit2. Derive power/fuse/topology values from the device rather than copying a
single-slice constant into an unchecked context.

Preserve default-state bootstrap semantics: restore-inhibit initial context,
context-WA emission, switch-away and saved default state. Prove the minimal
command streamer path with a fresh ordered result and CSB first, then advance
to drawing. That intermediate result is explicitly not shader-rendering proof.

### Compiled shaders and draw state

Build a small offline NIR-to-Intel compiler harness within Mesa's Meson graph.
Use Mesa26.1.8 consistently for compiler metadata and shader packet mappings;
do not mix private structs with the older26.0.7 research reference. Local source
archive SHA256 is
`b320f65874fd9653ac6c0bd1616605387344e1247411a50c797b5f3fb9dc0b55`;
it matches the [official release checksum](https://docs.mesa3d.org/relnotes/26.1.8.html).
Use the pristine archive for the host compiler and record any build-only changes.

Construct a pass-through vec4-position VS and a constant-color FS using NIR
builder, preprocess through `brw_preprocess_nir`, and call `brw_compile_vs/fs`
for PCI46d1. This invokes the real Intel compiler without requiring a GL context,
LLVM renderer or a complete Iris backend. GLSL ingestion can follow through Mesa.

Save reproducible shader inputs, exact Mesa identity, EU binary and explicit
pointer-free metadata: program sizes, dispatch widths/offsets, GRF starts, VUE/URB
layout, inputs/barycentrics, scratch/constants and relocations. Initially require
zero scratch and no unresolved relocations. Never serialize a struct containing
host pointers as a native ABI.

Use Intel IGT Gen12 render setup and Mesa/Iris packet-field mappings as donors.
The IGT render-copy path disables VS and uses RECTLIST: it is not a triangle
pipeline. The first draw must enable the compiled VS and FS, use TRILIST and
matching vertices/viewport/clip/SF/SBE/URB/binding/target state. Packet generation
must enforce resource bounds and alignment before publication. All addresses
remain kernel-owned for the initial fixed workload; there is no raw-batch ioctl.

### Completion, reuse and error handling

Use the RCS `PIPE_CONTROL` flush/invalidate and post-sync breadcrumb sequence;
BCS `MI_FLUSH_DW` is insufficient for render/texture/instruction visibility.
Accept completion only after current-context CSB switch-away, current ordered
sequence, render-cache visibility, expected result and safe idle/error/owner
checks, then normal forcewake release and final DMA revalidation.

Keep the device lifetime/admission policy shared across BCS/RCS administrative
power boundaries, but retain engine-specific state, ledgers and reset domains.
After uncertain RCS work, stop admission and retain every referenced allocation.
Only a proven RCS recovery and reinitialization may allow reuse; otherwise reboot
remains recovery. Do not reset other engines or clear shared errors to hide a fault.

### Native proof and presentation

The first substantive milestone is a shader-produced triangle in owned memory:
verify selected interior/exterior pixels and protected padding against known
inputs after RCS completion. Keep CPU-generated expected results clearly separate
from the GPU output. Record actual RCS submission/CSB/sequence, shader identity,
readback and errors so CPU fallback cannot be mistaken for GPU rendering.

Then repeat with changing inputs in one boot and present completed snapshots
through the existing window's synchronized BBitmap path. Rendered output must
reach the CPU-visible snapshot only after retirement; failures use independent
CPU fallback and retain uncertain GPU backing. Preserve working BCS copies and
driver quiesce/resume/client teardown behavior.

## Phase B: Mesa/Iris native backend

After Phase A, design and implement general owned BO allocation/mapping/GPU-VA,
RCS contexts, residency/access validation, real fences/waits/errors, device and
topology queries and context-loss handling. Reuse Mesa's compiler, ISL/genxml and
Iris hardware-state logic. Audit direct DRM and common Intel calls outside
`iris_kmd_backend`, not just its function table. Preserve version consistency.

Connect the Haiku frontend/resource presentation path to actual Iris resources
with correct layout/resolve, format and synchronization. Start one real GPU GL
context and verified draw in a native window. CPU llvmpipe remains an explicit
fallback; selecting Iris by name alone is not success. Focused application and
Piglit/CTS coverage belongs after this hardware-backed GL path exists.

Phase A does not provide GPU OpenGL; the Mesa backend remains the agreed follow-on.
Vulkan and a full desktop compositor are separate work.

## Acceptance evidence

| Required outcome | Evidence |
| --- | --- |
| Faithful RCS context/init | Source-pinned construction checks plus native engine/owner report. |
| Real shader compilation | Reproducible Intel compiler build, inputs, code and explicit metadata. |
| Actual GPU drawing | Native RCS completion and verified render-target pixels; no CPU substitute. |
| Repeat and presentation | Changing-input draws in one boot, readback/window and bounded lifetime. |
| Failure handling | Focused uncertain-completion/lost-state check with retained memory. |
| Mesa backend | Actual Iris context/resource/submission/wait/presentation and verified GL draw. |
| Preserve working system | BCS copy, display/component hashes, app_server and lease postchecks. |

## Source map for implementation

- Linux v6.6 `gt/intel_lrc.c`: context-offset decoder/table, initial state and
  indirect workaround construction; `gt/intel_sseu.c`: topology/RPCS.
- `gt/intel_gt.c`: default-context bootstrap; `gt/intel_workarounds.c`: ADL
  context/engine/general WA; `gt/intel_reset.c`: render-domain reset.
- `gt/gen8_engine_cs.c`: RCS flush/breadcrumb and AUX; `intel_uncore.c`: forcewake.
- Mesa26.1.8 `src/intel/blorp/blorp_brw.c`: small compiler and NIR VS examples;
  `src/intel/compiler/brw`: compiler library/API; `iris_state.c`: VS/PS mapping.
- IGT1.28 commit`31ec677ca24e7ed86e35f367f40a29d3d9f51c06`,
  `lib/rendercopy_gen9.c`: Gen12 render setup reference, not a drop-in VS triangle.

The user approved this design and explicitly requested implementation. No RCS
hardware operation or dependency installation occurred during design investigation.
The physical N100 owner hash matched lifecycle003 at the baseline inspection.
