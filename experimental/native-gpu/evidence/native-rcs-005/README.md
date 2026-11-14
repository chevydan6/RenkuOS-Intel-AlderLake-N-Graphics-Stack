# Native RCS acceptance — corrected candidate 005

The Intel N100 executed the approved native render-engine foundation on RenkuOS:
compiled vertex/fragment shaders, verified changing-input triangles, retirement
and bounded reuse, synchronized window presentation and CPU fallback. This is a
fixed native workload; Mesa/Iris frontend integration follows separately.

## Validation

Native SDK compilation and `_KERNEL_` export linking passed, with 18 focused
fixtures. Host compiler-artifact, XML packet/decode and mutation checks passed.
Independent reviews covered hardware source mapping, ownership/lifetime,
completion, presentation and the narrow PCI interrupt-status correction.

The first boot repeated the RCS/BCS boundary script: bootstrap/draw and BCS copies,
quiesce, rejected suspended draw, resume, rejected draw until explicit bootstrap,
rebootstrap/draw, and further BCS copies. Resume advanced generation without
consuming a GPU sequence. The driver remained Ready at sequence 10, no failures.

The root-only completion-observation check then hid the next DRAW's CSB tail.
Valid GPU commands were unchanged. The request returned B_TIMED_OUT at stage 10,
CSB deadline error 3, after 23,764 us. All 4 KiB of output were zero; backing was
retained, state became Lost, completed stayed 10 and failed became 1. A later DRAW
was rejected without advancing sequence 11. `timeout-output-check.cpp` records
the exact check. This proves bounded handling of missing completion evidence,
not reset recovery from a physically hung render engine. Reboot recovered it.

The final boot completed 2,078 accepted GPU requests without failure, including:

- Four verified changing-input draws and BCS copies before the window.
- A bounded window with **1,800 GPU frames, zero CPU fallback frames**.
- A second window closed while producing: **266 GPU frames, zero CPU frames**;
  close joined the worker before destroying window/session resources.
- BCS copies and another verified RCS draw after both windows closed.

The sequence passed the descriptor-tag wrap boundary. Every successful draw
checked 654 safe interior/exterior/border samples, ordered nonce/complement and
sequence, class-0 activation/switch-away, visibility, idle/errors, owner/DMA and
normal power release before exposing the snapshot. Hashes for variants 0–3 were
28616d2d, 814e0c35, 77323fd5 and d8b0a07d (FNV over 1,024 words). Window log hashes
use bytes and therefore differ.

The prior initial-admission failure was reproduced and diagnosed in
[004](../native-rcs-004/README.md). The corrected RCS adapter ignores only volatile
PCI interrupt-status bit 3 during full initial captures. Raw all-ones and every
other bit/field remain strict; it adds no retry, PCI write or BCS change. The final
bounded run passed the previously observed failure counts; it is not an
exhaustive stability or conformance result.

## Final machine state and identities

State Ready, sequence/completed **2078**, failed **0**, last status 0; the query's
open accounts for clients=1. Both windows closed; app_server remains PID127.
All 12 baseline components outside the intentional GPU-driver replacement match,
including the original accelerant. Installed owner, bridge and preferred graphics
driver hashes match. The physical lease `native-rcs-001-20261114` was released
**after verified postchecks**. Display/HDMI and boot media were preserved.

| Installed component | SHA256 |
| --- | --- |
| Owner | b5e15f1aebfdaf46ee41e7d1ace683d7e10227e671208cd5f70257a8ce92183c |
| Bridge | 7149dedbec26c504ebc4f314de5d49d779fa311b78b1bc563affd0bcd541eb3a |
| Preferred graphics driver | 74ab6275cbfbe0f49bf4996d7310ebcdfb279e2bb9b86a3c251cf94507f98929 |

The native bundle is `/boot/home/native-rcs-owner-005`. Run
`build/render_window --frames 120` or `build/gpu_render_sample draw 0`. After
administrative resume, the CLI requires `bootstrap` first; the window handles
that explicit mode when the default is invalid. `--cpu` avoids GPU access.

Full exact-source/build/log archive on host and N100:
`native-rcs-final-evidence-005.tar.gz`, SHA256
`6e102a2b27ef9a16c371225d5d5b0e708cf4e5a9cd58ac214d6be54c12a7282a`.
`payload.sha256` identifies candidate inputs. The repository removes one trailing
space-only source line after the build; executable code is unchanged.

## Scope

No OS kernel/ACPI changes, hardware OpenGL/Vulkan, general shaders/commands,
zero-copy scanout or GPU desktop composition. Presentation reads back a 32×32
B8G8R8A8 image into B_RGB32/app_server. Whole-machine cache flushes remain; this
run is not a performance benchmark. DMA remains limited to the observed
VT-d-translation-disabled configuration. The fixed GPU pool is boot-retained;
uncertain completion requires reboot for this milestone.

Next, map Iris BO/VA/context/submission/fence and resource-presentation contracts
onto this proven native foundation, before claiming hardware OpenGL coverage.
