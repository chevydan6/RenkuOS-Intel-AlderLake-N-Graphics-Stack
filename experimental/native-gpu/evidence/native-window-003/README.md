<!-- SPDX-License-Identifier: MIT -->
# One native window presenting BCS buffers — 2026-11-14

Base: `80dec26`, branch `codex/i915-native-adaptation`.
The user explicitly selected the BCS-buffer presenter now, with shader rendering
connected later. Source: [buffer_window.cpp](../../buffer_window.cpp).

**A native BWindow now consumes verified BCS copy results through BBitmap and
app_server, with a worker thread, bounded frame handoff and CPU fallback.**
This is a CPU-readback presentation bridge. Source pixels are generated on CPU;
BCS copies them. No shader rendering, zero-copy scanout, accelerated app_server
composition or performance improvement is claimed.

## Run

Native source, executable and logs: `/boot/home/native-window-003`.

```sh
cd /boot/home/native-window-003
./buffer_window
```

The button switches between BCS copies and CPU presentation while the worker is
running. `--cpu` starts without opening the GPU endpoint. `--frames N` produces a
bounded number of frames and then leaves the last image visible until the window
is closed; its mode control is disabled when the producer is finished.
`--test-fallback-after N` deliberately disables the client GPU path after N good
GPU frames, before submitting another request. It does not inject a hardware
fault or claim to reproduce an uncertain GPU completion.

The normal bundle build now produces `build/buffer_window` with native G++:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. buffer_window.cpp -lbe -o buffer_window
```

No kernel, accelerant, app_server or installed library replacement/reboot is
needed for this application; it uses the existing native_execution/v2 ABI.

## Buffer, format and synchronization contract

1. A worker owns the native buffer session. It creates two fixed 4 KiB objects,
   uploads a new opaque 32×32 source and makes one synchronous BCS copy request.
   The native owner builds commands and retains its pinned backing as before.
2. `CompletionGate` requires the current monotonic sequence and matching ordered
   breadcrumb, nonce/complement, successful native retirement, idle/error state
   and full 1024-word data proof. Only then does the worker read back and compare
   every byte with the current source. Failed upload/copy/completion/readback
   closes the session and permanently disables this client's GPU submissions.
3. After successful readback, a BMessage owns a copy of the CPU-visible pixels.
   A semaphore allows one outstanding frame message. The worker never writes
   the window's bitmap and retains no pointer into it. UI invalidations may
   coalesce; the latest accepted complete frame is drawn.
4. The window thread alone copies the snapshot into its B_RGB32 bitmap using
   the bitmap's actual BytesPerRow. Input format is explicitly opaque BGRX8888
   on the little-endian N100, with checked dimensions, stride and bounds.
   Unsupported formats are rejected. No alpha-blending or arbitrary image-layout
   conversion is promised. The focused policy smoke covers padded rows and
   invalid format/stride rejection.
5. `Draw()` uses ordinary **synchronous** `BView::DrawBitmap`, not DrawBitmapAsync.
   The pinned RenkuOS implementation sends the draw then calls Sync(), so this
   UI thread does not overwrite the bitmap while its preceding draw is pending
   consumption. See [BView::DrawBitmap](https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/kits/interface/View.cpp#L3137).
   app_server then performs its normal drawing/presentation; its backend is not
   replaced by this demo. This is not a vblank or tear-free presentation contract.
6. CPU fallback uses the current CPU source copied into independent CPU memory.
   Neither its message nor its BBitmap aliases the possibly uncertain native
   GPU backing. Closing the driver session frees CPU-only staging; the resident
   owner retains GPU-visible memory under its existing fail-stop rules. There
   is no automatic retry of a failed GPU path.
7. Closing the window signals the worker, wakes a pending frame-slot wait and
   joins the worker before view/session/semaphore destruction. The join may wait
   for the existing bounded synchronous native operation to return; normal
   drawing does not run that operation on the UI thread.

`presented frame=...` logs mean a snapshot was accepted into the UI bitmap and
scheduled for drawing, not that every frame has a separate physical scanout or
vblank timestamp. `job_us` measures the worker's generation/copy/readback job,
excluding UI queue/draw/app_server time. The 33 ms pacing delay is a demo setting,
not an FPS guarantee or new performance benchmark.

A later shader producer can reuse this CPU-visible snapshot/window consumer.
It still needs its own render-completion and format/layout contract; the current
BCS-specific completion gate must not be relabeled as an RCS fence implementation.

## Actual native evidence

The final native build passed with `-Wall -Wextra -Werror`; the portable policy
smoke passed on both host and RenkuOS ([build log](build.log)).

- [GPU window](gpu-window.jpg), [log](gpu-window.log): four accepted GPU frames,
  sequences 632–635, with 1024-word copy/readback verification and stride 128.
  The last frame remained visible; normal close joined the producer.
- [Fallback window after resize](fallback-window.jpg), [log](fallback-window.log):
  four GPU frames at sequences 636–639, then eight CPU frames after the explicit
  client fallback check. The last GPU sequence stayed 639: no later GPU request
  was made. The window remained functional, resized correctly and closed cleanly.
- [Visible mode control](mode-window.log): a native run started in CPU mode.
  Actual NanoKVM mouse clicks selected BCS copies and then CPU again. The window
  accepted 101 frames: 31 GPU and 70 CPU. Closing while the producer was active stopped
  it and joined it normally. These counts are functional evidence, not throughput.
- The focused smoke rejects a stale completion, latches submission disablement
  and rejects later otherwise-valid completions. No actual GPU timeout, hardware
  fault, device removal or quarantine/recovery experiment was performed.

Two actual UI failures were corrected before the final checks. A paused resize
left old drawing fragments ([initial screenshot](initial-resize-bug.jpg));
`B_FULL_UPDATE_ON_RESIZE` now redraws the size-dependent image and labels.
The full drawing view originally overlapped the mode control as a sibling and
intercepted its mouse hits. The button is now a child of that view; the pinned
[BWindow hit-test traversal](https://github.com/RenkuOS/Source/blob/4799bb5a188c4bb6bbef967287640cc21a061a0d/src/kits/interface/Window.cpp#L3999)
and the final actual mouse test confirm the correction. Initial candidates and
logs are retained in the full archive. Static review found no remaining material
lifetime, concurrency, format or fallback issue after these corrections.

## Current state and artifacts

[Exact source/binary hashes](payload.sha256), [hash verification](source-check.log),
[kernel](kernel.log), [unchanged resident owner/bridge](installed-after.log),
[unchanged original display/observer components](unchanged-after.log),
[app_server PID127](app-server.log) and
[no remaining presenter process](presenter-processes-after.log) are retained.
Executable SHA256:
`b0094b411a28ea6235ea71826c34bf80920cbd7228e4212e59aad219979bf335`.
Native wall time is incorrect; hashes identify the final build.

The installed BCS owner/bridge and original thirteen components are unchanged;
no source was written into RenkuOS/Source. The physical lease
`native-window-20261114-001` was released after postchecks. HDMI build057 and
NanoKVM boot media were preserved. The native copy owner remains usable after
these successful requests without reboot.

Full native sources, binaries and initial/final logs:
`/home/dan/projects/native-window-evidence-003.tar.gz` and target
`/boot/home/native-window-evidence-003.tar.gz`, SHA256
`bd7ca0ae79407a5eb091ad8398e982e456e671710e7bed51db007c3a650754c4`.

## Remaining desktop work

The bridge is deliberately limited to the current 4 KiB buffer API and a 32×32
opaque image scaled by ordinary app_server drawing. It does not enable a GPU
compositor or replace back-to-front desktop copies. Larger render surfaces,
shader production, cross-engine fences, additional formats and a validated
presentation/scanout interface remain separate work. Use the
[desktop integration decision](../../../../research/n100-submission/DESKTOP-INTEGRATION-INVESTIGATION.md)
and [render-engine phase](../../../../research/n100-submission/MESA-RENDER-ENGINE-PHASE.md)
for those boundaries. Previous measurements still show that tiny BCS copies
through this prototype cost more than CPU copies; this demo does not change that
result or justify automatic GPU desktop presentation.
