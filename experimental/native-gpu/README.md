<!-- SPDX-License-Identifier: MIT -->
# Native Intel GPU copy sample

Active direction: native adaptation retaining Intel hardware logic. See
[the current handoff](../../NATIVE_GPU_HANDOFF.md).

RequestImage.h reuses the existing native Gen12 context/CSB/nonce helpers and
adapts Linux v6.6's XCS completion sequence for a polling native request.
prepare_module.cpp connects that image to the existing GGTT memory owner and
the validated native page allocation bridge. Native compilation/export linking
and preparation passed earlier; the integrated native GPU copy now passes on
the physical N100. See [actual copy evidence](evidence/native-copy-005/README.md).
See [native-prepare-001](evidence/native-prepare-001/README.md).

The focused host image smoke and one native preparation call pass. No engine publication occurs in that preparation endpoint;
the mapping/v1 contract remains memory-only. The separate execution prototype below supplies actual DMA admission, execution
ownership, engine programming and completion. This memory-only preparation
endpoint itself still performs no GPU work.

Hardware sequence reference: Linux v6.6 commit
ffc253263a1375a65fa6c9f62a893e9767fbebfa,
drivers/gpu/drm/i915/gt/gen8_engine_cs.c and gen8_engine_cs.h,
gen12_emit_fini_breadcrumb_xcs/tail, gen8_emit_wa_tail and
__gen8_emit_flush_dw/gen8_emit_ggtt_write. The polling adaptation uses a NOOP
instead of requesting an interrupt that the native display handler does not own.

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.c
Ported-from-license: MIT

Ported-from: Linux/v6.6/drivers/gpu/drm/i915/gt/gen8_engine_cs.h
Ported-from-license: MIT

The reused native helpers retain their own original licenses and provenance in
workstreams/gpu-next and research/n100-submission. New native integration is MIT.

BcsEngine.h adds fixed BCS0 initialization and two-port ELSQ publication from
Linux v6.6 intel_execlists_submission.c (MIT). It is now called by the retained native
execution owner. The actual kernel SDK compile and native simulated-IO smoke
pass; see [BCS engine evidence](evidence/bcs-engine-001/README.md). Its caller
must supply the retained ownership, forcewake and memory/DMA/cache/TLB policy.
The earlier engine-only checkpoint itself made no hardware submission.

## Retained native execution prototype

NativeExecution.inc is included into the resident IntelMemoryOwner image. Its
new native_execution/v1 endpoint owns one fixed nonce + 4KiB PPGTT copy attempt
per boot. It reuses the mapping lease/pin, existing forcewake/reset helpers,
request/context image and linear BLT builder. The user client supplies no command
stream, GPU/CPU address, buffer or callback. All request backing remains owned
until reboot in this initial prototype, including after success. This is bounded
experimental retention, not a general allocation/retirement API.

The direct-DMA backend requires the actual boot's DMAR coverage and all enumerated
VT-d units to have translation disabled; it never disables an enabled IOMMU. The
private page hierarchy excludes firmware RMRRs. The owner programs the pinned
Gen12 MOCS/PAT profile, flushes CPU stores, invalidates BCS translation, initializes
BCS/HWSP/CSB and loads ELSQ. Failure keeps backing and any held power requests.
The fixed workload copies a kernel-generated pattern between two private pages,
checks every output dword, and separately checks nonce/complement, ordered
breadcrumb and context transitions. gpu_copy_sample.cpp is the runnable client.

Prepare from the exact installed owner source (SHA guarded):

```sh
python3 experimental/native-gpu/prepare-execution.py --owner /path/to/IntelMemoryOwner.cpp --output /path/to/new-bundle
```

Transfer the resulting archive, extract into a new native directory, then run
`bash build-execution.sh`. This checks native kernel compilation/export linking
and one CPU image smoke. `bash install-execution.sh` backs up the baseline owner
and installs the new owner/bridge. **Reboot before invoking**
`build/gpu_copy_sample`: B_KEEP_LOADED prevents file replacement from updating
resident code. Keep the existing NanoKVM physical lease/recovery procedure.
The installer requires the known baseline owner; it does not overwrite a backup.
An updated resident owner needs another controlled reboot before running new code.

No successful GPU copy is claimed by compilation. Physical attempts/results and
current installed binary hashes are recorded in the handoff and evidence.

Physical execution005 now returns `GPU 4KiB copy sample: PASS`: all1024words,
nonce/complement, ordered sequence and CSB activation/completion match, followed
by normal forcewake release. The actual native command was:

```sh
cd /boot/home/native-gpu-execution-005
build/gpu_copy_sample
```

See [logs and hashes](evidence/native-copy-005/README.md). The sample's request
backing remains retained until reboot; this is copy-engine acceleration rather
than a complete render/shader or graphics API implementation.
