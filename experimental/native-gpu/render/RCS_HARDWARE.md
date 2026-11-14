# Offline N100 hardware profile

`RcsHardwareProfile::Build(snapshot, output)` is a pure, bounded selection of
Linux v6.6 i915 programming for PCI `8086:46d1`, revision zero, graphics 12.0.
It performs no IO and grants no ownership or submission permission. It replaces
`output` only on success. The snapshot is copied before writing the result.
The numeric table is complete for the selected source paths below; it is not a
complete native RCS initialization procedure. PAT/MOCS table installation,
forcewake, reset/interrupt handling, whitelist programming, context defaults,
and the native adapter remain separate obligations.

`intel_step.c:adlp_n_revids[0]` maps graphics/media to A0 and display to D0.
`i915_pci.c` maps ADL-N IDs to `adl_p_info`, so `IS_ALDERLAKE_P` branches apply.
No ADL-P non-N revision substitution is permitted.

The adapter obtains these reads under its separately proven MMIO/forcewake and
ownership contract; this helper does not perform or authorize those reads:

| Snapshot field | MMIO offset | Interpretation |
| --- | --- | --- |
| `identity`, `revision` | PCI configuration | Packed device/vendor `0x46d18086`, revision zero |
| `sliceEnable` | `0x9138` | `GEN11_GT_SLICE_ENABLE`, low eight bits must equal one |
| `geometryDssEnable` | `0x913c` | `GEN12_GT_GEOMETRY_DSS_ENABLE`, nonempty mask of six DSS slots |
| `euDisable` | `0x9134` | `GEN11_EU_DISABLE`, invert low eight bits; each enabled bit expands to two adjacent EUs |
| `mirrorFuse3` | `0x9118` | `GEN10_MIRROR_FUSE3`, invert low four bits for live L3 banks |
| `vcsMask` | derived from `0x9140` | Admitted engine presence: `(~GEN11_GT_VEBOX_VDBOX_DISABLE) & 0x5` for ADL-P platform engines VCS0/VCS2 |

The native owner must reconcile `vcsMask` with actual engine discovery; it is
not an ownership token. No VDBOX access may be inferred from owning RCS alone.
The helper rejects empty/multiple slices, empty/out-of-range DSS, all-disabled
EUs, no live L3 bank, unsupported VCS bits, and wrong identity/stepping.
Reserved high slice/EU/L3 fuse bits are ignored as in the source masks.

`intel_sseu.c:gen12_sseu_info_init` defines one actual slice, six maximum DSS,
16 maximum EUs per DSS, and slice-only power gating. The EU mask applies to
each live DSS. `intel_sseu_make_rpcs` uses the population count of the observed
validated slice mask: enable bit31, slice-count-enable bit18, and slice count
at bit12. Thus valid one-slice profiles produce `0x80041000`; DSS and EU counts
are not power-gating fields on this device.

The default MCR steering is slice zero and the lowest live DSS, not the highest
available instance: forcewake's minimum configuration may power only that DSS.
`intel_gt_mcr.c:intel_gt_mcr_init` and
`intel_workarounds.c:icl_wa_init_mcr` also check the L3-bank intersection.
`defaultSteeringL3Valid` indicates whether implicit DSS steering works for L3;
otherwise L3 reads require explicit bank steering to `l3SteeringBank`.
The adapter must serialize selector ownership and restore transient steering,
read a live instance with the proper steering class, and multicast MCR writes.
An MCR operation is not an ordinary unsteered MMIO RMW. Register `0xfdc`
preserves bit31 (multicast), replacing only bits30:24.

The table below is in ascending offset order within each Linux list.
`clear/set/read` retain `i915_wa` semantics. Masked16 `set` includes upper-half
write enables and must be written directly. Other RMW values are
`(old & ~clear) | set`. Verify only `(readback ^ set) & readMask == 0`;
a zero read mask deliberately disables verification. Context and engine/GT
lists must stay separate; identical register numbers across lists are not
merged. Within the engine list the sampler bits0/15 and ROW_CHICKEN2 bits8/14
are OR-merged exactly as `_wa_add` merges these disjoint entries.

| List / scope | Offset | Clear | Set | Read mask | Kind / selected source |
| --- | --- | --- | --- | --- | --- |
| context | `2580` | `0` | `00060002` | `6` | masked16; disable GPGPU mid-thread preemption |
| context | `6604` | `ffffffff` | `e0040000` | `0` | write-only FF_MODE2 GS224/TDS128 timers |
| context | `7018` | `0` | `20002000` | `2000` | masked16; Wa_1806527549 |
| context | `7300` | `0` | `00400040` | `40` | masked16; Wa_1606376872 |
| context | `7304` | `0` | `02000200` | `200` | masked16; CPS-aware color-pipe workaround family |
| engine | `2050` | `0` | `10801080` | `1080` | masked16; Wa_1607297627 |
| engine | `20a0` | `80000` | `80000` | `80000` | RMW; Wa_14010919138 |
| engine | `20c4` | `0` | `3fff0306` | `3fff` | masked16; CMD_CCTL UC read/write MOCS index 3 |
| engine | `20e0` | `0` | `40004000` | `4000` | masked16; per-context preemption policy |
| engine | `20ec` | `0` | `00020002` | `2` | masked16; FF DOP clock gate disable |
| shared render | `b004` | `80` | `0` | `80` | RMW; bus hash tuning |
| engine + shared render | `e18c` | `0` | `80018001` | `8001` | MCR masked16; smallPL + indirect state override |
| engine | `e48c` | `0` | `02000200` | `200` | MCR masked16; Wa_14010229206 |
| engine | `e4f4` | `0` | `41004100` | `4100` | MCR masked16; Wa_1606931601 + Wa_1409804808 |
| shared GT | `fdc` | `7f000000` | `lowestDss << 24` | `7f000000` | RMW; default MCR selector |
| shared GT | `9424` | `2` | `0` | `0` | RMW; Wa_14015795083, firmware may lock register |
| shared GT | `9550` | `200` | `200` | `200` | MCR RMW; Wa_14011059788 |
| shared GT, if VCS0 present | `1c3f10` | `400000` | `400000` | `400000` | RMW; Wa_14011060649 |
| shared GT, if VCS2 present | `1d3f10` | `400000` | `400000` | `400000` | RMW; Wa_14011060649 |

All offsets and masks come from Linux v6.6 `gt/intel_gt_regs.h`,
`gt/intel_engine_regs.h`, and `i915_reg.h`. Selection sources in
`gt/intel_workarounds.c` are `gen12_ctx_workarounds_init`,
`gen12_gt_workarounds_init` (including `icl_wa_init_mcr`, `__add_mcr_wa`,
`wa_14011060649`), `gt_tuning_settings` (no selected ADL branch),
`general_render_compute_wa_init` + `add_render_compute_tuning_settings`,
`rcs_engine_wa_init`, and `engine_fake_wa_init`.
`__intel_engine_init_ctx_wa` also calls `gen12_ctx_gt_fake_wa_init`:
nested-batch programming requires graphics >=12.55 and MOCS context programming
is COPY-only, so neither adds an RCS operation here. `intel_mocs.c:get_mocs_settings`
selects `gen12_mocs_table` with UC index 3. CMD_CCTL fields take doubled indices;
the profile does not install that table or assert it is installed.

Context workarounds must be emitted into an RCS bootstrap batch, using the
Linux `intel_engine_emit_ctx_wa` sequence: `emit_flush(EMIT_BARRIER)`, LRI of
the five sorted register/value pairs, MI_NOOP padding, then another barrier.
The Gen12 render barrier must supply the source-required PIPE_CONTROL flushing
and stalls. FF_MODE2 is unconditional write-only: CPU reads are unreliable
(Wa_1608008084), so never preserve read bits or verify this value. This helper
only returns the operation descriptions; it does not construct or execute the
bootstrap batch or accept a fence as proof that these operations were emitted.

Engine-list programming is reapplied after engine/render-domain reset. Shared
render entries are added once by Linux's `I915_ENGINE_FIRST_RENDER_COMPUTE`
owner and are lost on shared render reset. The combined sampler entry retains
both scope flags. GT-list registers survive an engine reset and are reapplied
at GT initialization/GT reset; their ownership spans the GT, including video
engines for Wa_14011060649. A native implementation must track invalidation at
these scopes, acquire all affected ownership and forcewake domains, program GT
steering before dependent engine MCR accesses, and establish defaults before
workload admission. Nothing here licenses a shared GT reset or write.

Focused offline verification:

```sh
c++ -std=c++17 -Wall -Wextra -Werror experimental/native-gpu/render/rcs_hardware_smoke.cpp -o /tmp/rcs_hardware_smoke
/tmp/rcs_hardware_smoke
```

The fixture checks every selected operation and scope, write-only behavior,
merged values, sparse DSS/L3 steering, full topology and one EU pair, optional
video-engine entries, observed-slice RPCS, and invalid-input output preservation.
