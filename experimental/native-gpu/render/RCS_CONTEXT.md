<!-- SPDX-License-Identifier: MIT -->
# CPU RCS context image

2026-11-14: standalone N100 revision0, graphics12.0, primary-GT RCS0
constructor. No live driver or BCS implementation changed. No hardware was
accessed. This is fresh restore-inhibited context construction, not default-state
bootstrap, saved render state, shader execution or permission to submit.

`NativeGpu::RcsContextImage::Build(const Input&, uint32_t*, size_t)` returns bool.
The destination must be dword aligned and exactly65536bytes. Input fields, in order:

```cpp
uint32_t identity, revision; // 0x46d18086, 0
uint64_t contextGgtt, ringGgtt, pml4Dma;
uint32_t ringBytes, tail, rpcs;
```

Context and ring GGTT ranges must be distinct, page aligned and below4GiB.
Ring size is a power of two from4KiB to2MiB; nonzero tail is8-byte aligned
and leaves64bytes. PML4 is a page-aligned39-bit DMA address. Zero DMA/GGTT
addresses are accepted as address shapes, not proof of a real allocation.
`rpcs` is copied unchanged: callers must independently derive and validate it
against the device's actual topology and power configuration. No scalar RPCS
value establishes hardware validity. All rejection checks precede destination
writes. Input is snapshotted before clearing, allowing input/output aliasing.

The layout contains14hardware pages including ppHWSP, indirect WA at page14,
and a terminating per-context batch at page15. Register state starts at page1.
The RCS table retains all five LRI groups, NOP gaps, repeated OA registers and
Gen11+ close BBE at dword0xb9. RPCS is dword0x43. The29-dword indirect WA
restores timestamp0x23, CMD_BUF_CCTL0xb7 and GPR0x75 using context-relative
GGTT addresses, invalidates RCS AUX0x4208 and pads to128bytes. The pinned
source's separate predicate helper is retained at WA-page byte2048; it is
outside that indirect span and is not linked by this constructor.

Source identities (Linux v6.6):

- `drivers/gpu/drm/i915/gt/intel_lrc.c`, SHA256
  `05867c37aa7cef99a3c636c14761ea31378a9408af7c882c235461b56e81fcb4`.
  `gen12_rcs_offsets`, `set_offsets`, `__lrc_init_regs`, `lrc_update_regs`,
  `gen12_emit_indirect_ctx_rcs`, `setup_indirect_ctx_bb` and predicate helper.
- `drivers/gpu/drm/i915/gt/gen8_engine_cs.c`, SHA256
  `9583d998fd94e7bf6c3dd7e17637a3c88cf315c5bba1f2a30bda6ccefd3c9d18`.
  `gen12_get_aux_inv_reg` and `gen12_emit_aux_table_inv`.

`check-rcs-reference.py` pins both source files and independently parses the
Linux RCS table into `rcs_linux_golden.h`; it does not inspect the constructor.
Use `--write` only to regenerate the checked-in reference from those pinned
sources. The fixture compares every byte of the64KiB image to this decoded
table plus separately specified initial values and WA commands. The reference
retains Intel MIT attribution.

Run from repository root:

```sh
python3 experimental/native-gpu/render/check-rcs-reference.py /home/dan/projects/linux-v6.6
c++ -std=c++11 -O2 -Wall -Wextra -Werror experimental/native-gpu/render/rcs_context_smoke.cpp -o /tmp/rcs_context_smoke
/tmp/rcs_context_smoke
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer experimental/native-gpu/render/rcs_context_smoke.cpp -o /tmp/rcs_context_smoke_san
/tmp/rcs_context_smoke_san
```

Observed: reference check, normal fixture and ASan/UBSan fixture PASS. Before
implementation, the same fixture compiled and failed its first valid-build
assertion against a false-return constructor (RED). Invalid-input preservation,
near4GiB GGTT,39-bit PML4, maximum ring/tail and aliased input checks pass.
Self-review compared register slots, MMIO mode flags, WA order and byte spans
against the pinned sources.

Eight existing native-gpu host smoke fixtures also pass with C++17, `-O2
-Wall -Wextra -Werror -Iworkstreams/gpu-next -Iexperimental/gen12`:
BCS engine, buffer copy, copy image, CSB cursor, lifecycle, presentation,
recovery and request image. The existing `pci_power_smoke.cpp` cannot compile
in this checkout because `IntelN100PciPower.h` is absent; this component does
not provide it. There is no unified native-gpu host test runner.

Caller responsibilities remain ownership/exclusion, topology/RPCS, pinned
backing, GGTT/PPGTT validity, DMA admission, cache/PAT/MOCS policy, context and
engine workaround programming, default-context bootstrap and publication.
No descriptor, ring publication, DMA allocation, topology discovery or live
render initialization is supplied by this CPU component.
