# Offline RCS0 publication foundations

`RcsEngine` is an offline-tested N100 rev0 (`0x46d18086`) publication helper.
It does not establish complete RCS initialization, execute a test on hardware,
or render. Construction performs no I/O and grants no permission.

The caller must first establish exclusive RCS ownership, a bounded reset,
forcewake, topology/RPCS, GT/engine/context workarounds, default-state bootstrap,
shared PAT/MOCS/TLB configuration, DMA admission and coherent owned mappings.
The initialized engine HWSP is separate from the context ppHWSP. Its CSB poison
and initial tail must match the explicit `initialCsb` argument (default 11).
Callbacks must return boundedly; `safe` revalidates the whole owner contract,
and `visible` provides actual store ordering and mapping visibility.

`Initialize(IO, identity, revision, statusGgtt, deadline, initialCsb=11)` checks
inactive/idle state, resets CSB pointers, programs HWSTAM/execlists/HWS, masks all
engine errors, clears EIR and rejects a nonzero ESR. It never unmasks IRQs.
`Publish(IO, descriptor)` clears unused ELSQ port1, writes port0 low/high, then
LOAD. Its report records the attempted offset/value and `loadAttempted` before
the callback, preserving ambiguous-write evidence. `Published` means LOAD was
issued, not completion. Retain backing and power after any attempted store until
the external retirement/reset contract permits release.

`Rearm(IO, deadline)` only renews a published helper's software budget (at most
100000 microseconds). The caller must prove ordered breadcrumb, CSB switch-away,
visibility and idle before calling it; rearm itself performs no MMIO/reset.
`intel_n100_rcs_descriptor` validates space for the complete 64 KiB RCS image,
4 KiB alignment, fixed identity/revision, nonreserved tag, class0/instance0 and
zero counter. It proves no ownership, image contents or DMA admission.

Shared `IntelN100ContextStatus::Arm(generation, tag, engineClass=3)` and
`IntelN100CsbReader::Run(..., initialTail=11, engineClass=3)` accept only class0
or class3, always instance0. RCS callers explicitly select class0; existing BCS
callers and captured BCS CSB records retain their class3 default. CSB evidence
alone never permits releasing GPU resources.

Source review uses Linux v6.6 commit
`ffc253263a1375a65fa6c9f62a893e9767fbebfa` under
`/home/dan/projects/linux-v6.6/drivers/gpu/drm/i915/gt/`:

- `intel_execlists_submission.c:726` (`write_desc`), `:908`
  (`execlists_submit_ports`): descending ports, low/high ELSQ writes and LOAD.
- `intel_execlists_submission.c:2807` (`reset_csb_pointers`), `:2888`
  (`enable_error_interrupt`), `:2931` (`enable_execlists`): cursor reset,
  HWSTAM, Gen11+ mode, STOP_RING clear, HWS posting read, EMR/EIR/ESR. The
  polling helper deliberately omits Linux's final instruction-error unmask.
- `intel_engine_cs.c:273` (`intel_engine_context_size`) and `:371`
  (`intel_engine_set_hwsp_writemask`): Gen11/12 render context is 14 pages;
  HWSTAM is per-engine on Gen6+. `intel_lrc.c:1072` context allocation and
  `:1360` RCS indirect context workarounds inform the separate context builder.
- `intel_lrc.c:1426` descriptor layout and `:1459` (`lrc_descriptor`),
  `intel_lrc.h:105` class/instance shifts; `intel_engine_regs.h:72`, `:81`,
  `:199` HWSTAM/error/mode register definitions at RCS base `0x2000`.
- `gen8_engine_cs.c:202` (`gen12_emit_aux_table_inv`) and `:247`
  (`gen12_emit_flush_rcs`) are RCS-specific command/flush requirements. This
  helper emits neither commands nor breadcrumbs; those remain caller work.

Focused offline checks (strict C++11; no native build or hardware access):

```sh
c++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iworkstreams/gpu-next experimental/native-gpu/render/rcs_engine_smoke.cpp -o /tmp/rcs_engine_smoke
/tmp/rcs_engine_smoke
c++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iworkstreams/gpu-next experimental/native-gpu/bcs_engine_smoke.cpp -o /tmp/bcs_engine_smoke
/tmp/bcs_engine_smoke
c++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iworkstreams/gpu-next experimental/native-gpu/csb_cursor_smoke.cpp -o /tmp/csb_cursor_smoke
/tmp/csb_cursor_smoke
```

## Offline RCS recovery

`RcsRecovery::{IO, Run, LastReport}` matches the `BcsRecovery` interface, with
RCS MMIO base `0x2000`, `MSG_IDLE_CS=0x8000` and render GDRST domain `2`.
The caller must prove exclusive RCS/shared render-domain ownership, all
forcewake, DMA safety and bounded callbacks for the whole 10 ms attempt.
The helper stops CS, disables prefetch, checks idle and refuses outstanding MI
forcewake. It follows CAT_ERROR/READY preparation, two Gen12.0 domain resets,
a 50 µs successful-reset settle, and cancellation after successful or failed
preparation/domain reset. Only disabled RING_CTL, idle and clear GDRST produce
`Quiescent`; failures retain uncertainty. This grants no power-release or free
permission and performs no full-GT reset or forced non-ready retry.

Linux v6.6 mappings: `intel_engine_cs.c:397` (`get_reset_domain`), `:1605`
(`__intel_engine_stop_cs`), `:1682` (`__cs_pending_mi_force_wakes`);
`intel_reset.c:268` (`gen6_hw_domain_reset`), `:562`
(`gen8_engine_reset_prepare`), `:603` (`gen8_engine_reset_cancel`), and `:656`
(`skip_reset` cancellation); `intel_gt_regs.h:658,666,918` define render BIT1
and MSG_IDLE_CS. Pending forcewake is conservatively rejected; this helper
does not reproduce Linux's shared-domain GPM completion procedure.

```sh
c++ -std=c++11 -Wall -Wextra -Werror -pedantic experimental/native-gpu/render/rcs_recovery_smoke.cpp -o /tmp/rcs_recovery_smoke
/tmp/rcs_recovery_smoke
```
