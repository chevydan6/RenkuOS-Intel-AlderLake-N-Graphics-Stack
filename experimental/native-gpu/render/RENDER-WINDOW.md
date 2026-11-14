# Native RCS window presenter

`render_window.cpp` requests the fixed kernel-owned RCS shader triangle and presents a validated CPU-visible snapshot through a `B_RGB32` bitmap and app_server. It makes no OpenGL or desktop GPU composition claim.

Build on the native Haiku SDK from `experimental/native-gpu`:

```
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. render_window.cpp -lbe -o build/render_window
```

Run `build/render_window --frames 120` for bounded production; the last frame remains visible until the window closes. `--cpu` makes no device requests. `--test-fallback-after 4` disables the client after four validated GPU frames; its label explicitly identifies this as a client check, not a GPU fault. Fallback is latched after the first GPU failure, closes the descriptor and independently generates a CPU grid image.

Interface assumptions: ioctl `0x91504e30`, version 1, a 4352-byte command with a 240-byte report and 4096-byte output; PROBE=1, BOOTSTRAP=2, DRAW=3. Probe must grant the known owner/profile. Missing DEFAULT_VALID causes one bootstrap. Successful bootstrap/draw has public COMPLETE stage 12, current increasing sequence, matching nonce/complement/observed sequence, class-0 instance-0 activation and switch-away for tag `1+(sequence-1)%0x7fe`, valid state bits and no reported error. Draw variants 0..3 return linear 32x32 B8G8R8A8, pitch 128. The target hash is FNV over 1024 little-endian **words**. Packed XY is `(y<<16)|x`. Userspace independently checks the hash, sample counts, safe triangle/interior/exterior samples and key pixels against `RcsDrawImage` constants. Edge bands are omitted from geometric comparison to avoid rasterization edge rules. The kernel still owns visibility, retirement and backing lifetime; userspace never uploads a GPU source image.

One semaphore permits one outstanding frame message. The worker owns the descriptor and ioctl storage; messages copy completed pixels. Only the window thread writes the bitmap, copies each row using its actual stride and synchronizes app_server bitmap drawing. Closing joins the worker before destroying view/session resources. Frame pacing is about 33 ms plus work, not a benchmark.

Portable policy fixture (no GPU or native UI evidence):

```
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Iworkstreams/gpu-next experimental/native-gpu/render/rcs_presentation_smoke.cpp -o /tmp/rcs_presentation_smoke
/tmp/rcs_presentation_smoke
```

Native build, actual shader output and window captures are recorded in [candidate003](../evidence/native-rcs-003/README.md). Lifecycle and failure validation are recorded separately; CPU fixtures alone do not prove hardware behavior.
