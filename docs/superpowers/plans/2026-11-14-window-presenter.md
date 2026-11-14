<!-- SPDX-License-Identifier: MIT -->
# Native BCS window presenter implementation plan

**Goal:** Present completed BCS buffer results in one native window, with correct
opaque BGRX32 pixels, ordered completion and a CPU fallback. Shader rendering
connects later, as explicitly selected by the user.

**Architecture:** A worker owns the existing native buffer session and performs
one synchronous GPU copy at a time. A bounded message handoff copies completed
pixels to the window thread, which alone updates its B_RGB32 bitmap and draws
through app_server. On failure the client stops GPU submissions and presents
CPU-generated pixels from memory never mapped by the GPU. The kernel remains
unchanged; this is a readback presentation bridge, not zero-copy or compositing.

**Tech stack:** C++17, BeAPI BApplication/BWindow/BView/BBitmap/BMessenger, native
buffer-copy v2 ABI and existing resident owner.

**Spec:** User-selected BCS-buffer presenter; existing
`experimental/native-gpu/BUFFER_COPY.md` and desktop/render investigation docs.

## Work

- [x] Add one portable frame/retirement policy and a focused smoke proving stride,
  invalid format/stale completion rejection and failure latching.
- [x] Add the native application: worker, bounded immutable frame messages, UI
  thread bitmap ownership, CPU/GPU mode control, resizing and orderly shutdown.
- [x] Native compile; demonstrate GPU frames followed by a bounded client-side
  fallback check, without inducing an actual GPU fault or resetting the driver.
- [x] Capture native window/log evidence, check source/binary hashes and display
  component hashes, review lifetimes, update handoff and commit.

## Limits and review focus

- Current buffer ABI is two 4 KiB objects; present 32x32 opaque tiles, scaled by
  app_server. CPU generates source pixels; BCS copies them, no shader claim.
- CPU access follows successful native retirement and readback. Copying the
  bitmap uses its actual row stride. Unsupported formats are rejected.
- One outstanding window frame; no unbounded queue or worker writes to UI bits.
- Close requests stop and join the worker before UI/session state is destroyed.
- A failed GPU path is not retried. Native backing remains resident according to
  the existing failure policy; CPU fallback uses independent memory.
- Desktop compositing, larger buffers, direct scanout and performance optimization
  remain later work. Existing timings do not justify automatic tiny GPU copies.

## Execution record

Implemented inline in the existing task checkout. The policy smoke first failed
for the absent PresentationFrame.h, then passed on host and native SDK. Native
compilation and client linking passed with -Werror. Actual paused resize
and mouse-control failures were diagnosed against pinned BeAPI source and fixed
with full resize invalidation and correct parent/child control hierarchy. Final
candidate003 passed GPU-only, client-fallback, actual mode-switching and active
shutdown checks. Evidence is experimental/native-gpu/evidence/native-window-003.
The existing kernel/accelerant/app_server were not changed. No hardware fault
injection or zero-copy/rendering claim was made.
