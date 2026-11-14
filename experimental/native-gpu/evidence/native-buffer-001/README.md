<!-- SPDX-License-Identifier: MIT -->
# Native owned-buffer copy interface — 2026-11-14

Base: `27be1f6`, branch `codex/i915-native-adaptation`.
Intel N100 / 8086:46d1 revision0, RenkuOS hrev60072+518.
See [API and lifetime rules](../../BUFFER_COPY.md).

Native owner and bridge compiled/export-linked successfully and were installed
with their previous binaries saved under `/boot/home/native-buffer-001`.
A controlled reboot loaded the matching native_execution/v2 pair.
[Build log](build.log) records focused admission/lifetime, image, PCI-anchor and
CSB checks. No general IOMMU support or unrelated hardening was added.

[Runtime log](runtime.log) records `build/buffer_copy_sample` passing three
17×5 copies, pitches80/96bytes, at sequences1–3. Every one of85pixels matched;
bytewise readback also verified that all destination padding/remaining bytes
retain0xa5. The same sample rejects same-buffer and out-of-bounds copies before
the first valid submission, and rejects the handles through a fresh session
with no created buffers. Success is printed only after all these checks pass.
Then `build/gpu_copy_sample` passes the original1024dword copy at sequence4.
All execution reports are status0/stage14 with matching ordered sequences,
EIR/fault0. Whole-buffer hashes need not match for rectangular copies.

The [resident owner and bridge hashes](installed.sha256) match the built files;
[all binary hashes](final-binaries.sha256), [exact payload hashes](payload.sha256),
[source check](source-check.log), [kernel](kernel.log),
[unchanged original display/observer hashes](unchanged-after.log) and
[app_server PID127](app-server.log) are retained. HDMI build057 and boot media
remain unchanged. Lease `native-buffer-20261114-001` was released after postchecks.
No physical timeout/fault injection was performed. The admission/lifetime smoke
checks rejection and CpuOwned→GpuInFlight→CpuOwned/Unknown transitions; static
review found no material lifetime or validation issue.

Full source, objects, binaries and logs remain at
`/home/dan/projects/native-buffer-evidence-001.tar.gz` and target
`/boot/home/native-buffer-evidence-001.tar.gz`, SHA256
`205be266c86c4f38307f3abb3d9301faec52c158f370a3a447733d50ea198fbe`.
The native wall clock is incorrect; source and binary hashes identify the build.

Only the resident owner's pinned pages enter the GPU address space. Per-open
CPU staging buffers can be freed on final cookie release because they are never
published or retained by the owner. GPU buffers remain pinned even on successful
close and are reused after proven retirement. An uncertain execution blocks all
further GPU work and keeps affected session buffers unavailable until close;
reboot remains recovery. The direct-DMA path still refuses enabled VT-d
translation and never programs general IOMMU mappings.
