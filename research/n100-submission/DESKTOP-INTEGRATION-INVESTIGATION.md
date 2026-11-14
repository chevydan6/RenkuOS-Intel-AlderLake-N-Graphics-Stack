<!-- SPDX-License-Identifier: MIT -->
# Desktop integration decision from the native BCS measurements

Investigation: 2026-11-14. GPU branch baseline: `23580ff`.
RenkuOS source is pinned to `4799bb5a188c4bb6bbef967287640cc21a061a0d`.
This is a source investigation and integration recommendation, not a desktop
acceleration implementation or a new hardware experiment.

## Decision

**Keep the existing CPU desktop path enabled. Investigate a narrow, batched
back-buffer-to-scanout presentation interface next. Do not implement only the
legacy screen-blit hook and call that desktop acceleration. Defer a general
GPU-backed bitmap/offscreen rendering interface until the presentation boundary
and its costs are understood.**

The measured prototype takes about 2.66–2.68 ms per complete GPU copy versus 10–89 ns
for warm CPU memcpy at 64 bytes–4 KiB. Its upload/copy/readback path has similar
fixed cost. These measurements support rejecting tiny per-rectangle GPU
submissions with the current implementation. They do **not** compare CPU writes
to the actual scanout mapping, measure larger surfaces, isolate BCS execution,
or establish a crossover for presentation. See the
[benchmark, boundaries and raw evidence](../../experimental/native-gpu/evidence/native-benchmark-003/README.md).

The first presentation candidate should preserve CPU painting into the current
RGBA back buffer, use persistent owned GPU staging buffers, batch damage into
one bounded request where possible, and copy directly to an explicitly leased
scanout destination. It must eliminate destination CPU readback from the present
operation. CPU fallback remains the default until an equivalent end-to-end
presentation benchmark demonstrates benefit.

## What the pinned source actually does

The user-supplied finding is confirmed, with these additional boundaries:

| Stage | Verified path | Consequence |
|---|---|---|
| Allocate drawing buffer | `AccelerantHWInterface::SetMode()` creates `MallocBuffer(frontWidth, frontHeight)`; `MallocBuffer` uses `malloc(width*4*height)`, reports `B_RGBA32` and stride `width*4`. [S1], [S2] | CPU memory, with no GPU allocation/import contract. |
| Attach renderer | `HWInterface::DrawingBuffer()` selects BackBuffer when double-buffered. `DrawingEngine::FrameBufferChanged()` attaches Painter to it; Painter retains the raw CPU bits/stride in its AGG rendering buffer. [S3], [S4], [S5] | Changing a blit hook does not redirect painting. |
| Bitmap drawing | `DrawingEngine::DrawBitmap()` calls `fPainter->DrawBitmap()` inside a dirty transaction. [S4] | The demonstrated BCS copy does not replace alpha blending, scaling or general rasterization. |
| Publish ordinary damage | `DrawTransaction` destruction calls `InvalidateRegion(fDirty)`. `Window` suppresses immediate copies during an update and `EndUpdate()` calls `DrawingEngine::CopyToFront(dirty)`, which also calls `InvalidateRegion()`. [S4], [S6] | There are existing damage/update boundaries worth preserving. |
| Present region | `InvalidateRegion()` iterates each rectangle through `Invalidate()` → `CopyBackToFront()`. The local accelerant override of `_CopyBackToFront()` delegates to the base implementation. [S1], [S3] | There is no GPU presentation in this inspected path. |
| Move pixels to front | `_CopyBackToFront()` computes each source address using screen coordinates and stride; `_CopyToFront()` uses row-wise CPU `memcpy` for RGB32/RGBA32 and CPU conversions for other formats. [S3] | A narrow raw 32-bit GPU path must retain CPU conversion for other front formats. |
| Cursor ordering | `CopyBackToFront()` clips damage, locks floating-overlay/cursor state, excludes the software cursor region, copies the remaining region, then calls `_DrawCursor()`. [S3] | An asynchronous replacement cannot return as though copying is complete. |
| Scroll/window moves | `DrawingEngine::CopyRegion()` orders rectangles and calls CPU `CopyRect()`, then invalidates the touched area. `_CopyRect()` selects row direction and `memmove` for overlapping moves. [S4] | Current non-overlapping native copies are not a drop-in scroll implementation. Updating scanout alone would leave the authoritative back buffer wrong. |

The main local path is therefore:

```text
CPU Painter/CopyRect
    -> malloc-backed RGBA DrawingBuffer
    -> DrawTransaction / Window::EndUpdate damage
    -> DrawingEngine::CopyToFront / HWInterface::InvalidateRegion
    -> CopyBackToFront (clip, cursor/overlay handling)
    -> CPU row copies or conversion into accelerant framebuffer
```

This describes the inspected local `AccelerantHWInterface` backend, not every
virtual/test backend or a runtime trace of the installed app_server binary.
The working N100 display also has local adaptations; its exact active layout and
cache policy still need runtime confirmation before any scanout-writing trial.

## Why the legacy hooks are not this integration

`intel_extreme/hooks.cpp` exports `B_SCREEN_TO_SCREEN_BLIT`, fill and invert,
alongside the engine/synchronization hooks. The inspected local app_server
`_SetupDefaultHooks()` and `_UpdateHooksAfterModeChange()` do not look up these
2D hooks; their lookups cover mode/framebuffer, cursor, power/brightness and
overlay operations. Searching that local-interface source subtree for the
screen-blit/acquire/fill/wait hook constants finds no consumer. [S1], [S7]

There **is** a consumer in `BWindowScreen::_SetupAccelerantHooks()` and
`CardHookAt()`: the legacy direct-screen/game API. Implementing a working hook
could benefit those explicit clients, but would not wire ordinary app_server
painting or back-to-front presentation to BCS. [S8]

The legacy screen-blit signature carries an engine token and rectangle list,
not arbitrary owned source/destination buffers. Intel's current legacy
`xy_source_blit_command` sets source base equal to destination base, both the
framebuffer offset. That cannot describe a malloc-backed app_server source. Its
legacy synchronization also does not supply the validated native request
lifetime: the engine token advertises no 2D capability, `intel_get_sync_token()`
returns success without filling a token, and `intel_wait_engine_idle()` returns
early for the LAKE/SOC0 families. These are reasons to retain the working native
Gen12 request/retirement logic rather than transplant the old ring path. [S9], [S15]

## Existing memory interfaces are not a ready-made offscreen solution

- `RenderingBuffer` exposes CPU `Bits()`, format, stride and dimensions. It has
  no GPU handle, mapping generation, CPU/GPU access transitions or fence API.
  Painter retains its pointer, so adding a fence inside `Bits()` alone would not
  bracket subsequent CPU rendering. [S5], [S10]
- `BitmapManager::CreateBitmap()` uses client allocator storage for ordinary
  client bitmaps, malloc for server-only bitmaps, and a distinct overlay path.
  `CloneFromClient()` clones a client area; it does not establish GPU DMA
  admission or read/write ownership. General offscreen acceleration consequently
  reaches beyond the desktop's single `MallocBuffer` allocation. [S11]
- `AccelerantBuffer::SetOffscreenBuffer(true)` is only a pointer-view flag:
  `Bits()` adds `virtual_height*bytes_per_row` to the framebuffer address. It
  allocates/pins/maps nothing and establishes no lifetime. The inspected Intel
  mode path allocates `bytes_per_row*virtual_height` for the framebuffer itself;
  the flag is not proof that a second surface exists after it. [S12], [S13]
- Intel's `get_frame_buffer_config()` supplies a CPU framebuffer pointer, stride
  and the legacy aperture-derived `frame_buffer_dma` value. Mode setting frees
  and replaces the framebuffer allocation. None of those fields is an owned
  PPGTT handle or permission for our experimental module to write scanout. [S13]
- Direct clients and screenshots matter: `ServerWindow::HandleDirectConnection()`
  passes the **front** buffer to `DirectWindowInfo`, which exposes its bits/area
  and stride. `DrawingEngine::ReadBitmap()` also reads front-buffer CPU bits.
  New pending GPU writes must be reconciled with those accesses and mode-change
  notifications. [S14], [S16], [S4]

The current native copy API has two fixed 4 KiB private data pages, origin-to-origin
copies, separate CPU staging arrays and synchronous destination copyback. Its
private PPGTT addresses are not aliases of scanout. Even the existing PPGTT tree
builder is bounded to 16 data pages; screen-sized backing is a real extension,
not just changing a buffer-size constant. See
[CopyRequestImage.h](../../experimental/native-gpu/CopyRequestImage.h),
[OwnedBuffers.h](../../experimental/native-gpu/OwnedBuffers.h) and
[IntelN100PpgttTree.h](../../workstreams/gpu-next/IntelN100PpgttTree.h).

## Integration options

| Option | What it buys | Cost and decision |
|---|---|---|
| Implement only legacy screen-blit/fill hooks | Acceleration for explicit legacy hook consumers. | No automatic normal-desktop path; wrong source/buffer model and old synchronization. Do not select for this goal. |
| CPU back buffer + persistent staging + batched presentation | Keeps Painter and current RGBA allocation; snapshots immutable damage for GPU reading; targets the actual back-to-front copy. | Adds a CPU staging copy, requires large/batched requests and a leased scanout destination. **Selected next investigation/prototype boundary**, conditional on measured benefit. |
| CPU-mappable GPU-owned RGBA back buffer | Could remove the staging copy while preserving a CPU pointer for Painter. | Requires driver-owned allocation/mapping, cache/access transitions and CPU-writer exclusion; cannot simply swap `MallocBuffer` or use the offscreen flag. Evaluate after the staged baseline. |
| General GPU-backed ServerBitmap/offscreen drawing | Could eventually accelerate reusable bitmap operations and avoid transfers. | Much broader client/shared-area, synchronization, drawing-dispatch, overlap and lifetime changes. Defer. BCS alone does not implement full rendering. |

The selected boundary should be an optional Intel presentation capability
through the accelerant/native owner, with the generic CPU path retained. No
numeric hook or new public BeAPI ABI is proposed here. Capability absence,
unsupported formats or a measured CPU-faster case should select CPU before GPU
publication. Use the resident native BCS owner for hardware access and retirement;
do not add a second independent register/ring owner in app_server.

## Concrete requirements for a presentation prototype

1. **Batch at the region boundary.** `InvalidateRegion()` is virtual, so a local
   backend can preserve a whole region instead of inheriting its per-rectangle
   loop. Preserve the single-rectangle `Invalidate()` entry too, and account for
   `CopyRegion()`'s direct per-rectangle calls. `_CopyBackToFront()` alone is too
   low to recover the original damage batch. `Window::EndUpdate()` offers an
   existing per-window update boundary, not a proven whole-desktop frame queue.
   Preserve exact damage/cursor exclusion; copying a bounding box across holes
   is not automatically equivalent, especially with direct front-buffer clients.
   Coalescing across windows/frames is additional scheduling work. [S3], [S6]
2. **Preserve the authoritative CPU image.** Snapshot clipped damage into
   persistent pinned staging, with explicit source/destination coordinates,
   independent strides, format and 64-bit bounds validation. Initially keep the
   submit/retire call synchronous so the caller cannot rewrite its source before
   the snapshot is complete and existing cursor ordering remains understandable.
   Snapshotting isolates later GPU reads; it does not by itself serialize all
   other front-buffer writers.
3. **Lease scanout from its real owner.** Bind an opaque destination to the
   active mode/allocation generation, actual pitch, format, memory layout and
   validated GPU address. Keep its backing alive through retirement and drain
   before mode replacement/free. Do not infer identity DMA from a CPU pointer,
   `frame_buffer_dma` or a GGTT offset. Preserve the observed VT-d-disabled
   admission restriction; general IOMMU support stays separate. [S13]
4. **Use the correct visibility contract.** CPU dirty stores must be visible to
   BCS, BCS writes visible to scanout, and subsequent CPU reads/cursor writes
   ordered after completion. Determine cache types/aliases before replacing the
   prototype's whole-machine `wbinvd`; this investigation does not establish a
   safe per-range substitute. BCS retirement alone is not a vblank/page-flip
   contract and does not promise tear-free presentation.
5. **Preserve cursor, overlay and CPU readback ordering.** Carry clipping and
   software cursor exclusion/drawing into the batched operation. Handle direct
   front-buffer clients and `ReadBitmap()` explicitly. Existing drawing locks
   include shared/read access; a read lock is not blanket exclusion of CPU
   writers, nor can it safely be assumed upgradeable to an exclusive present
   lock. Audit the relevant call sites before introducing asynchronous work.
6. **Make failures observable.** `_CopyBackToFront()` returns void, the parent
   returns success after calling it, and `DrawingEngine::CopyToFront()`/dirty
   transaction destruction do not propagate an execution error. A new backend
   needs a defined completion/failure path. After ambiguous GPU publication to
   scanout, ordinary CPU fallback to that same destination is not established
   recovery: the GPU may still write it. Retain backing and stop GPU presentation
   until proven retirement/reset; reboot can remain prototype recovery. CPU
   fallback for requests rejected before publication is a separate safe case.

These are integration contracts to resolve, not implementations claimed by this
investigation. In particular, there is no presentable-scanout handle or batch API
in the currently installed experimental module.

## The next evidence to collect before enabling desktop GPU copies

Keep the desktop on CPU while measuring its actual presentation workload:
damaged bytes, rectangle count, source/destination strides/formats and CPU time
in the back-to-front copy. Distinguish drawing time from presentation time, and
identify the active backend/cache mapping on the N100 build.

Then extend the private owned-buffer prototype to larger, persistent surfaces
and batched rectangles, initially using private destinations. Compare CPU copying
into a destination with the **same cache/mapping characteristics** as presentation
against staging + visibility + submission + wait/retirement. Measure representative
large damage and many-small-rectangle batches; report setup separately and full
end-to-end latency and CPU time consumed by waiting, rather than multiplying small-copy throughput into a claimed
screen rate. Existing 64 B–4 KiB warm CPU measurements are not that comparison.

Only after the ownership/visibility contract and those measurements justify it
should a bounded, opt-in RGB32/RGBA32 scanout presentation trial replace the
corresponding CPU copy. Keep CPU rendering and unsupported-format conversion.
A general GPU offscreen renderer is not the prerequisite for this experiment.

## Source evidence and verification

All claims above were checked against the pinned local Git objects. The
[21-file manifest](desktop-integration-004/source-manifest.json) records Git blob
IDs and SHA256 hashes of the actual inspected bytes. The back-buffer allocation
excerpt was also retrieved through the GitHub connector at the same pinned ref,
blob `6b122d67ef172e8461c7445cbc9ba0c43ce36aa6`, matching the local source.
Independent review rechecked the source claims and all 21 manifest entries
against the pinned Git objects and found no material issue.
No source/binary in RenkuOS/Source, app_server, accelerant or kernel was modified.
No native build, runtime damage trace, scanout write or new hardware test was
performed. The performance inputs are the already-recorded native benchmark.

[S1]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/interface/local/AccelerantHWInterface.cpp
[S2]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/MallocBuffer.cpp
[S3]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/HWInterface.cpp
[S4]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/DrawingEngine.cpp
[S5]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/Painter/Painter.cpp
[S6]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/Window.cpp
[S7]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/add-ons/accelerants/intel_extreme/hooks.cpp
[S8]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/kits/game/WindowScreen.cpp
[S9]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/add-ons/accelerants/intel_extreme/engine.cpp
[S10]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/RenderingBuffer.h
[S11]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/BitmapManager.cpp
[S12]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/drawing/interface/local/AccelerantBuffer.cpp
[S13]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/add-ons/accelerants/intel_extreme/mode.cpp
[S14]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/ServerWindow.cpp
[S15]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/add-ons/accelerants/intel_extreme/commands.h
[S16]: https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/servers/app/DirectWindowInfo.cpp
