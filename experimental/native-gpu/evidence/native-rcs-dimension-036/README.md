<!-- SPDX-License-Identifier: MIT -->
# Mesa 26.3 packet layout for a 64×64 N100 target

The pinned Mesa 26.3.0 `gen120_pack` build regenerated the current 32×32 RCS
draw image. All `Batch`, `Dynamic`, `Surface`, `Vs`, and `Fs` words match the
previous committed image exactly; only its generator-version comment changed.
The canonical [32×32 header](../../render/draw/RcsDrawGenerated.h) and manifest
were refreshed from that build. The CPU draw-image smoke passed.

The same emitter, compiled with `--dimension 64`, produced the isolated
[64×64 packet artifact](../../render/draw/RcsDrawGenerated64.h) and [manifest](manifest64.json).
`verify-dimensions.py` passed with only these expected changes:

| State | 32×32 | 64×64 |
|---|---:|---:|
| Viewport center/scale | 16 | 32 |
| Viewport, scissor, surface, drawing max | 31 | 63 |
| Surface pitch minus one | 127 | 255 |
| Target GPU pages at 0x406000 | 1 | 4 |
| Post-sync scratch GPU address | 0x407000 | 0x40a000 |
| General/instruction address-space pages | 8 | 11 |

The shader arrays remain identical. Exact changed dword indices are enforced
by `render/draw/verify-dimensions.py`; [manifest32.json](manifest32.json) and
[manifest64.json](manifest64.json) retain input and output hashes. The current
owner has eight mapped draw pages and a 4 KiB ioctl output, so this 64×64
artifact is **not** submitted to hardware. The next implementation boundary
is a separate owner/ABI version with four target pages, a non-overlapping
post-sync page, 16 KiB readback, and full completion/pixel validation. The
working 32×32 owner path is unchanged.
