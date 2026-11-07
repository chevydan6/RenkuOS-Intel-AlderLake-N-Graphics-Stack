<!-- SPDX-License-Identifier: MIT -->
# GART aperture caller audit

2026-11-07. Searched the complete Git HEAD tree with git grep, including blobs
outside the sparse checkout, for aperture callbacks and B_AGP_GART_MODULE_NAME.
This is an in-tree audit, not a guarantee about external binary clients.

Intel Extreme is the production caller of map_aperture/unmap_aperture. Its two
initialization unwind sites and final uninit discard the old aperture ownership;
a later init obtains a fresh ID. Diagnostic reporting has replaced silent unmap
failures. The common graphics device cookie is shared between opens, so per-open
buffer ownership must not be imposed on those legacy ioctls.

The other AGP module consumers found were intel_810, nvidia, radeon, radeon_hd
and via. No aperture callback invocation was found in those driver trees; module
reference usage alone is not an aperture owner. The in-tree gart_tester is the
other map/unmap caller. It performs terminal unmap before dropping the module,
without using the aperture afterwards. It contains stale-looking callback usage
and is not accepted as current native validation evidence.

Remaining ownership problem found: after failed unmap, allocate_memory and
bind_aperture still admitted new resources into the retained record. Automatic
reopen cleanup could then destroy that newly accepted work. Patch 0074 rejects
those two creation paths with B_BUSY while cleanup is pending, under sLock.
Queries and explicit free/unbind/unmap cleanup remain available; a successfully
recreated aperture has a fresh ID and clean state. Retry does not adopt old work.

The host allocation fixture checks unchanged output addresses and zero allocation
calls for this rejection. The actual bind callback is tested after a faulted
unmap in the lifecycle fixture. Current host suite passes 154 existing checks
plus cleanup-state/rejected-bind assertions; original regression control retains
its 39 expected failures. Native build 072, bundled tests, 17 recovery cases and host finalizer pass.
The disposable VM shut down cleanly. Evidence is in
installation/private/gpu-hdmi-build-049/.

This admission rule does not make raw CPU mappings or future GPU DMA safe after
teardown. Callers must stop using backing before requesting unmap. Before GPU
execution, a separate kernel-owned request lifetime must keep all mappings pinned
through completion or proven reset. Device/module unload and hidden partial-bind
ownership remain open. No physical hardware validation is claimed here.
