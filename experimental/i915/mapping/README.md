<!-- SPDX-License-Identifier: MIT -->
# Owned GGTT memory experiment

This adds a **memory-only** kernel interface to the retained native N100 owner.
It allocates four non-reserved GART objects (4K ring, 4K batch, 4K result, 64K
context backing), returns their GPU global virtual offsets and kernel CPU aperture
aliases, and releases them without a second aperture owner or parallel PTE writer.
The labels describe backing roles; no valid hardware context or command stream
is constructed. No GPU command publication is permitted through this interface.

The candidate is deliberately outside `patches/`: native production integration,
DMA admission and execution lifetime remain separate work. Existing patches and
Linux imports are unchanged. The source baseline is RenkuOS
`4799bb5a188c4bb6bbef967287640cc21a061a0d` plus patches 0001 through 0096,
then `gart-invalidation.patch` and `owner-mapping.patch`. Patches 0097 onward
are separate ACPI/kernel experiments and are not prerequisites for this candidate.

The private `bus_managers/intel_memory_owner/mapping/v1` module gets v0 once,
sharing the actual retained owner and GART aperture. A new Request lease and
operation pin span the entire borrowed set. Display and observation leases are
never supplied by clients. GART owns physical allocation, placement, 64-bit PTE
encoding/stores, invalidation and unbinding. The owner validates bounds against
the aperture's reserved prefix and rejects overlapping returned objects before
publishing any descriptor. The exported pointer is a kernel-only clone of the
existing aperture's device cache. Native VM cloning preserves WC/UC attributes.
The original GART address is retained separately and is the only address freed.

A failed begin publishes no descriptors and conservatively retains its lease,
known buffers, CPU alias and possible hidden backing until reboot. This includes
closure during allocation. A failed end records successful frees individually,
retains remaining resources and permits retry without double free. The caller
must end all CPU accesses before calling end. Resident ownership prevents caller
unload from deleting retained records. The interface does **not** support device
publication: completion/reset ownership must be designed before that changes.

For N100, the native GART flush now retains its existing `wbinvd` backing cache
writeback and issues an uncached `GFX_FLSH_CNTL_GEN6` (0x101008) bit-0 store,
followed by a posting read. The existing strongly UC register/PTE mappings and
manager serialization are reused. The hardware register/ordering reference is
pinned Linux v6.6 `drivers/gpu/drm/i915/gt/intel_ggtt.c:gen8_ggtt_invalidate`
and `intel_gt_regs.h`, commit `ffc253263a1375a65fa6c9f62a893e9767fbebfa`.
This is a native implementation of that register operation; no new Linux source
body is imported or relabeled. Other platforms retain their previous path.
A posting read is not evidence of an executed GPU command or all engine TLBs.

From this repository:

```sh
python3 experimental/i915/mapping/prepare-bundle.py \
  --source /path/to/RenkuOS-source --output /new/bundle
python3 experimental/i915/mapping/run-host-tests.py \
  --source /new/bundle/source --output /new/host-results
```

Host checks compile the actual mapping function bodies and real lease class,
with mocked GART/area services under UBSan. They cover bounds, overlap, every
allocation/free failure, unchanged failed outputs, reentry, display closure,
kernel alias permissions, alias deletion failure and cleanup retry. Three
mutated generated copies must fail assertions for wrong CPU address domain,
wrong free address domain and lost successful-free records. Set
`SANITIZER_LDFLAGS` when using locally extracted sanitizer libraries.

Transfer the prepared archive and verify `payload.sha256`. In the actual target
Terminal, run `bash -x mapping/build-native.sh`. This builds against the installed
native SDK and never installs anything. After staging/reboot verification, run `bash -x mapping/run-native-tests.sh`
in that visible Terminal. It installs only the diagnostic, records a fresh result
directory, and waits for current unload evidence plus an absent device.
Physical staging needs the exact current
binary hashes, preserved originals, the existing physical lock, a controlled
reboot and a target-specific rollback script. Do not blindly replay a previous
staging script. No test requires changing the boot media or kernel.

The probe driver uses only kernel-owned descriptors. Users receive statuses,
GGTT offsets and sizes, never pointers or leases. It writes two CPU patterns
through all 19 pages, drains WC stores with `sfence`, reads all buffers after
all writes to detect cross-buffer aliases, exercises rejected operations, then
ends the set. Four runs expect 38927 checks each. It is **not** `gpu_exec_probe`.
The first physical attempt exposed an SMAP fault from direct access to the
user-permission aperture; the corrected owner uses the kernel-only alias.
Preserve that failure evidence along with successful results.

A passing test establishes coordinated native GGTT allocation, kernel CPU
aperture access/readback and explicit release for this sandbox. CPU physical and
per-device DMA addresses are not exported or assumed interchangeable. It does
not establish IOMMU identity, DMA readiness, GPU cache/PAT/MOCS correctness,
engine translation invalidation, context validity, submission or retirement.
Existing timeline/ring-space imports remain unresolved. See the new mapping
checkpoint in `PORT_PROGRESS.md` and its physical evidence directory.
