# Native RCS shader and window evidence — candidate003

Physical Intel N100 PCI8086:46d1 revision0, RenkuOS hrev60072+518. The approved
native RCS foundation runs actual compiled vertex/fragment shaders, independently
of Mesa/Gallium frontends. Mesa26.1.8 supplies the Intel compiler and command
packing definitions; this is not Iris, hardware OpenGL or Vulkan.

## What ran

- PROBE granted the private display-profile handoff: slice1, DSS3, EUmask3f3f,
  24EUs, RPCS80041000; reported engine/shared faults were zero.
- BOOTSTRAP sequence1 completed with class0 CSB activation/switch-away, current
  nonce/complement/sequence, visibility, idle and normal power release.
- Nine changing-input shader draws, sequences2..10, matched all654 safe
  interior/exterior/border samples each. Four target hashes were28616d2d,
  814e0c35,77323fd5,d8b0a07d (word-wise FNV). Uncertain edge bands are excluded.
- Existing BCS rectangular copies at sequences11..13 passed afterward.
- A native window presented12 validated GPU snapshots, sequences14..25.
  Another presented4GPU then8independent CPU frames via an explicitly labeled
  client fallback check, sequences26..29. Both closed and joined the worker.
- A continuous window presented1536GPU frames, sequences30..1565. The next
  request failed, the presenter latched CPU fallback, and the driver entered
  Lost with backing retained. The window subsequently closed normally.

## Observed failure and limits

`failure-snapshot.log` is the exact cached240-byte CPU report from resident
owner003, recovered under the exclusive physical lease using a temporary,
read-only diagnostic bridge. It required terminal Lost before copying metadata;
no GPU/MMIO access, reset or free occurred. The diagnostic driver was removed.

Request1566 has stage1, mapping0, dma0, retained1, pool-ready state, identity0,
and no forcewake/engine/CSB activity. The initial PCI Capture rejected before GPU
publication. That candidate did not record its precise Capture result or raw
reads, so the differing PCI field is unknown. This is not evidence of a shader
hang. Later diagnostics preserve the conservative rejection and record the
reason; no unchecked retry or stability relaxation follows from this run.

This run proves rendering and fail-stop CPU fallback, not continuous stability,
actual hardware-fault recovery, zero-copy presentation or desktop compositing.
The presenter reads back a32x32 B8G8R8A8 target into B_RGB32/app_server. Whole-machine
cache maintenance remains. DMA requires the observed VT-d-disabled configuration.

## Identities and raw evidence

Owner SHA256: d4934e2275a1fc8a959a121d598f89fc038726a63b97a1cbe0ebf67c120939ce.
Bridge: a2e57027e8e7f2d3ef172f17cb0589c73a78e02fa5cedf92ce714b7f3bcfdc4b.
Graphics driver:74ab6275cbfbe0f49bf4996d7310ebcdfb279e2bb9b86a3c251cf94507f98929.
See installed.sha256 and payload.sha256 for exact paths/source inputs. The
preferred graphics driver is the user-config copy, which shadows system copies.
The accelerant and display programming are unchanged; the GPU driver adds only
private acknowledgment of its existing display-only profile.

Full archive: `/home/dan/projects/native-rcs-shader-evidence-003.tar.gz` (also
`/boot/home/native-rcs-shader-evidence-003.tar.gz` on N100), SHA256
`a86f812940b14ae10623435baadc608268f71c1dbe2d767cd4743b8bd64a52a9`.
Initial bootstrap002 archive SHA256:
`d25529b895b7e97a4ae11fb53696f90d55bbaa7935183206f30df8e151b3cf5d`.
