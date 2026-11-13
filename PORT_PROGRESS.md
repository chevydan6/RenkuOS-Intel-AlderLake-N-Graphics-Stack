<!-- SPDX-License-Identifier: MIT -->
# i915 LinuxKPI experiment — 2026-11-13
Mission incomplete. Branch experiment/i915-linuxkpi starts at GPU branch commit
30dfdb32957215b4c59da3ac763ba14ab0c482d0. Existing display and patches are untouched.

## Established baseline
Read README, architecture, build instructions, native memory progress, first execution
plan and local RenkuOS-i915-N100-investigation.md. Native work already fixed 64-bit
GGTT, preserved HDMI build 057 and demonstrated physical forcewake/reset; execution
is still unproven. Reuse these results; do not repeat the investigation's older
32-bit-GART finding as if the native branch still lacks its fix.

## Compilation checkpoint
Pinned Linux v6.6 ffc253263a1375a65fa6c9f62a893e9767fbebfa.
Original intel_ring.c Git blob 59da4b7bd262058855d18023bbec71e8e0618793.
Whole original source fails at missing gem/i915_gem_internal.h (001).
A separate native KernelExport.h attempt fails at SupportDefs.h (002).
Initial extraction rejected an extra trailing newline (003); fixed imported bytes,
not the hash check. Source Git blob verification now passes.
Unchanged function bodies intel_ring_update_space, intel_ring_reset and
intel_ring_begin compile in a freestanding x86_64 host object (004).
Private slice object layouts replace full Linux objects explicitly.
This is extraction evidence, NOT whole-file LinuxKPI compatibility.
Native object attempt still fails at missing SupportDefs.h (005).
No linked module, native executable, GPU command, MMIO or DMA test was run.
gpu_exec_probe has not been fabricated or implemented.

## Reproduce
From experimental/i915:
    make host-object
    make native-object CC=<native-or-cross-gcc> NATIVE_CPPFLAGS='<matching SDK includes>'
Generated source, ELF object and dependency file are under build/.
nm shows i915_request_timeline, panic, renku_ring_wait_for_space unresolved.
No success stub resolves GPU wait/retirement.
Local workspace /home/dan/i915-linuxkpi contains only experimental additions,
not a clone of the full GPU repository (shell DNS unavailable).
GitHub connector was used to create the real branch and preserve its base tree.

## Next implementation task
Obtain matching RenkuOS development SDK/cross compiler or native VM access,
reproduce native-object, then implement request timeline/wait and retirement against
the retained completion path before linking ring reservation into a driver.
In parallel with that dependency closure, compile whole intel_ring.c with actual
GEM headers and inventory the next failures. Do not claim the partial layout is
binary-compatible with full Linux objects.
