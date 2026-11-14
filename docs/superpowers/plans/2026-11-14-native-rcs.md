# Native RCS implementation plan

> Execute with subagent-driven-development for independent offline tasks;
> integrate hardware work sequentially. Keep the user-requested existing branch.

**Goal:** Build a native Intel N100 GPU rendering engine with compiled-shader
drawing, repeated safe requests and window presentation; connect Mesa afterward.

**Architecture:** Separate RCS backing and engine-specific context/init/completion
from BCS. Reuse the resident owner, GART allocation, DMA admission and lifetime
policy. Generate shader code with Mesa's Intel compiler and draw commands from
upstream Intel hardware definitions. No raw user GPU command stream.

**Tech stack:** RenkuOS native C++, Linux v6.6 Intel reference, Mesa26.1.8 BRW/NIR,
IGT1.28 Gen12 render reference, focused host/native fixtures.

**Spec:** `docs/superpowers/specs/2026-11-14-native-rcs-design.md` (user approved).

## Global constraints

- PCI8086:46d1 rev0, graphics12.0; preserve Intel hardware logic and attribution.
- No OS kernel/ACPI work; preserve the known-working display and boot media.
- One request in flight, bounded polling; current VT-d-disabled restriction.
- Do not reset or publish RCS without exclusion of legacy ring writers.
- Shared GT state requires device-wide coordination; idle is not ownership.
- Focused validation and native proof, no exhaustive tests or broad GL claims.

## Review focus

- Context LRI gaps/register repeats and engine-specific value slots must match
  the pinned RCS source, rather than a modified BCS table (Task1 golden check).
- Compiler pointers/relocations must not escape into a native artifact (Task2
  artifact validation, including nonzero code and zero unresolved scratch/relocs).
- A legacy ring can be idle but still owned (Task3 admission-refusal check).
- A completed MI request does not flush render targets (Task4 completion packet
  and native pixel checks, not just a breadcrumb).
- A lost request must not reopen admission or expose uncertain output (Task5
  bounded failure and later-admission check).

## Task1: CPU RCS context image

Files: create `experimental/native-gpu/render/RcsContextImage.h`,
`rcs_context_smoke.cpp` and a small source-pinned reference generator/check.
Do not modify BCS helpers or live driver integration in this task.

Interface: `NativeGpu::RcsContextImage::Build(const Input&,uint32_t*,size_t)`;
input contains identity/revision, distinct context/ring GGTT ranges, PML4,
ring size/tail and caller-validated RPCS. Output is a64KiB fresh context including
14 hardware pages, one indirect WA page and one terminating per-context page.

- [x] Write a failing fixture checking exact decoded Linux v6.6 RCS LRI table,
  mode/ring/PML4/RPCS value slots, BBE at0xb9,29-dword WA padded to128bytes,
  RCS AUX0x4208, context-relative timestamp/CMD_BUF_CCTL/GPR0 restores.
- [x] Implement the constructor using pinned source logic and MIT attribution;
  validate shape/ranges before writes. State explicitly that it grants no
  ownership, topology validity, DMA or hardware submission permission.
- [x] Run focused valid/invalid construction checks and review independently.
- [x] Commit this independently testable CPU-only component.

## Task2: reproducible Intel shader compiler

Files: create `experimental/native-gpu/render/compiler/` with NIR compiler harness,
Mesa integration patch/build recipe, explicit artifact schema and focused checker.
Use a separate host build directory; do not modify the existing Mesa VM trees.

Interface: host executable `native-rcs-shaders OUTPUT_DIRECTORY` produces
`vs.bin`, `fs.bin`, NIR sources and pointer-free `metadata.json` for N100.
Metadata records Mesa/archive identity, key fields, program sizes, dispatch
width/offsets, GRF starts, VUE/URB layout, inputs/barycentrics, scratch and relocs.

- [x] Verify upstream Mesa26.1.8 archive checksum and prepare isolated build tools.
- [x] Write artifact checks that fail with no real compiled shader artifacts.
- [x] Build a vec4-position pass-through VS and constant-color FS with NIR builder,
  `brw_preprocess_nir`, VS VUE setup and `brw_compile_vs/fs`. Use the Mesa Meson
  dependency graph, not a hand-maintained compiler object list.
- [x] Reject unresolved relocations/nonzero scratch for this initial shader pair;
  serialize scalar fields/arrays explicitly, never raw program-data structs.
- [x] Compile and check actual nonempty Gen12 code and reproducible metadata;
  review independently and commit. This proves compilation only, not execution.

## Task3: ownership, resource layout and RCS engine initialization

Files: owner generation/integration, new render engine/profile headers and
`render/OWNERSHIP.md`. Add only the narrow handoff needed after source audit.

- [ ] Trace matching native display source and all primary-ring writers; record
  an enforceable exclusion contract before reset. The matching native ALD display profile already skips legacy ring allocation,
  setup/teardown and producer hooks; use that audited profile, not upstream
  callsites alone, to define private admission.
- [ ] Classify engine-local/shared GT stores and synchronize shared changes.
- [ ] Implement a bounded admission check that rejects missing/stale ownership;
  verify rejection without publishing GPU work.
- [ ] Allocate distinct RCS context/ring/HWSP/result and PPGTT resource backing
  through the existing GART/native page owner, with explicit size/layout ledger.
- [ ] Adapt N100 render reset/init, topology/RPCS, workarounds and default-context
  bootstrap from pinned Intel sources, retaining bounded fail-stop lifetime.
- [ ] Compile/export-link, acquire physical lease and prove one RCS command
  streamer request with ordered result/CSB/idle/error evidence. Preserve display
  postchecks. Mark this as command-streamer proof, not shader drawing.

## Task4: shader draw and pixel proof

Files: `render/RcsDrawImage.h`, focused image fixture, native render API/client,
`NativeExecution.inc` integration or a focused included RCS implementation file.

- [ ] Define a bounded non-overlapping resource layout for command/state/code,
  vertices, render target and results, using the compiled artifacts from Task2.
- [ ] Add a failing image fixture pinning alignment/bounds, VS/FS packet fields,
  TRILIST, vec4 positions, viewport/clip/SF/SBE/URB and target setup.
- [ ] Implement kernel-built draw packets with Mesa/IGT hardware logic; do not
  reuse the IGT disabled-VS RECTLIST assumptions.
- [ ] Implement Intel RCS PIPE_CONTROL flush/invalidate/breadcrumb completion;
  check lifetime and CPU visibility before exposing output.
- [ ] Execute one native triangle, verify expected interior/exterior/padding
  pixels, current RCS sequence/CSB and zero errors. Save exact source/binary/
  shader identities and native logs. A clear or CPU image is not acceptance.

## Task5: repeated draw, window and focused failure behavior

Files: native render client, window producer integration, lifecycle integration,
focused native evidence and `NATIVE_GPU_HANDOFF.md`.

- [ ] Repeated changing-input draws reuse only retired backing; preserve separate
  BCS state and integrate both engines with quiesce/resume/client teardown.
- [ ] Check one controlled uncertain completion: block/recover safely, retain
  referenced memory, never present a failed request as successful output.
- [ ] Present completed GPU snapshots through synchronized existing window path,
  with independent CPU fallback on failure.
- [ ] Verify working BCS copies, display/app_server and lease postchecks; obtain
  final independent review, commit evidence and report actual limitations.

## Mesa follow-on

After the RCS engine is proven, write the next implementation plan for native
Iris BO/context/query/submission/fence and resource presentation integration.
No GL application or conformance result counts as GPU evidence until that backend
selects actual RCS work and verified hardware output. Keep the overarching goal
active for any remaining user-requested Mesa integration.

## Execution ledger

- Base65d2c13; design approved by user: “yeah that works”, “implement that”.
- RCS source audit:14 hardware context pages, distinct RCS table/WA and completion;
  shared-GT and legacy-ring exclusion are explicit pre-publication gates.
- Mesa26.1.8 archive hash matches official release notes. IGT reference pinned to
 31ec677ca24e7ed86e35f367f40a29d3d9f51c06. No RCS hardware changes yet.

- Task1 implemented and independently reviewed: exact pinned register/WA image and strict C++11 fixture pass; CPU construction only. No RCS hardware operation.

- Task2 complete: real Mesa Intel compiler emits VS176bytes/FS112bytes; clean recipe, repeated byte-identical artifacts and focused checker regressions pass. Review fixed invalid VUE/SIMD-offset acceptance, scoped re-review clean.
- Task3 audit correction: matching native display sources already exclude ALD legacy ring writers; no display patch inferred. CPU ownership policy and hardware operation profile are subcomponents, not hardware admission proof.
