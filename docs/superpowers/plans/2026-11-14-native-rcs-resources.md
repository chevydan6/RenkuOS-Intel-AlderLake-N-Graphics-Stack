# Native RCS Resources Implementation Plan

> **For agentic workers:** Implement tasks in order in the current native
> adaptation branch. Preserve the installed `b591205` N100 regression state.

**Goal:** Give a native Vulkan userspace driver owned GPU buffers, GPU virtual
addresses, safe RCS submission, and completion without Linux DRM/GEM.

**Architecture:** Extend the existing RenkuOS kernel add-on and bridge. Keep
GPU page tables and physical backing in the owner, file-local handles in the
bridge, and hardware-independent resource semantics in a small shared ABI.
Use ANV's `anv_kmd_backend` as the workload contract, then implement Intel
packet details inside the N100 backend.

**Tech Stack:** RenkuOS/Haiku C++ kernel modules, Intel N100 PPGTT/RCS,
Mesa 26.3.0 ANV/NIR/Intel compiler references, C++ host and physical N100 tests.

**Spec:** `experimental/native-gpu/VULKAN-NATIVE-BOUNDARY.md` and the user's
Vulkan-only mission. No Zink or OpenGL work.

## Global constraints

- Preserve v1 fixed draws and v2 user-vertex draws as hardware controls.
- Do not modify the RenkuOS base kernel or add Linux DRM/GEM compatibility.
- No user-provided GPU address, DMA address, or command word reaches hardware
  before bounds, ownership, format, and packet validation.
- Resource pages remain owned until GPU retirement is proven; timeout is not
  proof of completion.
- Hardware-specific PPGTT and command encoding stay in the Intel backend.
- Measure canonical source bytes and model tokens separately from binaries.

## Review focus

- A forged or stale handle must not access another file's buffer.
- A size or offset overflow must fail before mapping or copying.
- Closing a file during submission must retain backing through retirement.
- A timeout must preserve ambiguous GPU-visible pages for recovery.
- Updating a PTE after first publication needs proven GPU TLB invalidation;
  until then reject the bind instead of silently reusing an address.

### Task 1: Resource and GPU VA contract

**Files:** Create `experimental/native-gpu/NativeResource.h`,
`experimental/native-gpu/resource_smoke.cpp`; modify the native build script.

**Interface:** `NativeGpu::ResourceTable` owns a file-local handle namespace.
`Create`, `Bind`, `Range`, `Address`, `Publish`, `Destroy`, and `RetireEpoch`
enforce 4 KiB GPU VA granularity, stale-handle rejection, and no PTE changes
after first publication. No ioctl or physical address is exposed in this task.

- [x] Write tests for allocation bounds, handle isolation, generation reuse,
  failed creation, and release; confirm the tests fail before implementation.
- [x] Implement the smallest CPU policy table that passes them.
- [x] Compile and run host and native CPU tests; commit the isolated change.

### Task 2: Owner-backed mapping

**Files:** Extend `workstreams/gpu-next/IntelN100PpgttTree.h` or add a focused
PPGTT mapper; extend the owner and bridge module interfaces and build script.

**Interface:** Define `native_gpu_resource_create`, `native_gpu_resource_io`,
`native_gpu_resource_bind`, and `native_gpu_resource_destroy` with file-local
handles. Create/map/read/write/destroy buffers through the private owner. The
owner allocates exclusive 4 KiB physical pages,
validates direct DMA, and binds an aligned range before first RCS publication.
No post-publication PTE mutation until a tested invalidation path exists.

- [ ] Test transactional allocation, map validation, per-file ownership,
  timeout retention, and cleanup before implementation.
- [ ] Implement the owner/bridge path and compile on RenkuOS without installing.
- [ ] Re-run v1/v2 regression before installing the isolated candidate.

### Task 3: Resource-backed triangle

**Files:** Extend the RCS batch builder and render request; add a native client
and physical regression runner.

**Interface:** A validated vertex BO supplies the vertex-buffer GPU VA. The
batch builder patches only the decoded vertex-buffer address and length fields;
shader and pipeline packets remain owner-selected.

- [ ] Test packet diff, BO bounds, malicious handles, and completion reporting.
- [ ] Run the candidate on N100 and compare pixels with the v2 control.
- [ ] Repeat v1/v2 and resource-backed draws; record hashes and raw traces.

### Task 4: Native command submission and fences

**Files:** Add a bounded submit ABI and RCS command validator; extend the
bridge, owner, and hardware runner.

**Interface:** Submit owner-validated command buffers with explicit resource
access and dependencies, return a fence/sequence, and expose wait/error state.
Preserve resource residency until completion or recovery.

- [ ] Reject forbidden packets and out-of-range relocations in CPU tests.
- [ ] Validate command execution, wait, readback, timeout, and recovery on N100.
- [ ] Map the resulting ABI to ANV's create/map/bind/queue hooks and measure
  the selected native build closure before claiming a Vulkan triangle.
