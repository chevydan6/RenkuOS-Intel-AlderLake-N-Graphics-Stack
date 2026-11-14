# Native Mesa shader compiler build on the physical N100

Date: host 2026-11-15 JST. Mesa source: pinned 26.3.0 archive SHA-256
`91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a`.
This experiment targeted the existing `native-rcs-shaders` host harness as a
first native Haiku compiler build. It did not install a Vulkan library, change
the N100 GPU driver, or run GPU commands.

## Native build reached Intel code

The physical RenkuOS install lacked Meson, Ninja, and Python. Its package
repository was reachable for search but refused package downloads. The exact
Haiku package files were fetched on the host, transferred with SHA-256 checks,
and installed in the user's **home** package location. Python 3.10, Mako,
PyYAML, and packaging were staged for Mesa's source generators. The installed
Meson package's `/bin/python3.14` launcher and the guest's clock skew needed
build-local wrappers and normalized timestamps in the isolated source copy.

Mesa configured natively with Gallium, Vulkan drivers, OpenGL, EGL, GBM, GLX,
LLVM, shader cache, zlib, and zstd disabled; Haiku was the selected platform.
The first `ninja -j 2` build progressed through NIR, ISL, and the Intel shader
backend to step **670/671**, linking `libintel_compiler.a`. SSH stopped
responding during this heavy build. A Govee outlet cycle recovered SSH, but
the persisted log ended at step 188; later buffered writes were lost. That
attempt did **not** produce a verified compiler executable.

A second run reconfigured Meson for `buildtype=release`, used `ninja -j 1`,
and flushed the filesystem periodically. During compilation, the N100 entered
the base kernel debugger. NanoKVM captured the [panic screen](block-cache-panic.jpg)
(SHA-256 `d55defb7320b72a92383663447721a6240407a311fe16f587fd6f33df5939004`).
It shows:

```text
PANIC: ASSERT FAILED (../src/system/kernel/cache/block_cache.cpp:3845):
block->previous_transaction == __null
```

The stack includes `BlockCache::Discard`, BFS block allocator clone creation,
BFS inode creation, and `file_create`; the current user thread was `cc1`.
This is a filesystem/kernel failure during a CPU compiler build, not evidence
of a GPU hang. No base-kernel fix was attempted under the current mission.

## Recovery state and limit

NanoKVM initially showed the debugger prompt. A typed reboot command arrived
as `eboot`, which the debugger rejected. The outlet was restored to **on**.
After a longer AC removal, NanoKVM again saw USB attachment and later a
[RenkuOS desktop](post-panic-desktop.jpg). Terminal launch displayed “Your
setting file could not be saved! (Initialization failed).” At the last check,
ping and SSH to
`192.168.1.114` were still unavailable. The physical system's filesystem and
installed driver functionality are therefore **unverified** after the panic.

Further large compiler builds should run on a disposable Haiku VM or a proper
cross toolchain. The physical N100 should be reserved for bounded GPU tests
after its boot and filesystem state are verified. The host compiler proof and
known-good GPU binaries in the repository were not modified by this attempt.
