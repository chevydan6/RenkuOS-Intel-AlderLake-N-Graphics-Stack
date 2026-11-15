# Native N100 UBO surface packet preparation (073)

Mesa ANV uses a RAW buffer surface for UBO loads on Gen12. The
[host packer probe](emit-surface.c), compiled with the pinned Mesa 26.3 build
flags by [build-probe.py](build-probe.py), produced a 16-byte RAW buffer state.
An [independent XML decode](verify-surface.py) checked the surface type,
format, 16-byte range, pitch, MOCS, base address, and disabled compression.
The [packed words](surface-words.log) use placeholder GPU VA `0x410000`.

The native [RCS patch helper](../../render/RcsUniformSurface.h) preserves
binding-table entry 0 for the render target, adds entry 1 pointing to the
UBO surface, and changes `3DSTATE_PS` from one to two entries. It also moves
the SIMD16 kernel pointer to offset 192, matching the [compiler metadata]
(shader-metadata.log): a 336-byte program, GRF start 2/2, zero scratch and
relocations. The [CPU smoke](../../render/rcs_uniform_surface_smoke.cpp)
checks exact packet changes and rejects a second patch. The updated
[Haiku VM owner build](owner-vm-build.log) passed its full smoke suite.

This helper is **not yet called by the owner**. No UBO shader was submitted to
the N100 and no installed binary changed. The next increment must bind the
actual UBO resource and approved shader BO to this state, preserve lifetime
through completion, and verify output from at least two different UBO colors.
The selected runtime closure remains 12,637,741 bytes because this helper is
not yet in the owner compilation closure. Model-token count remains unmeasured.
