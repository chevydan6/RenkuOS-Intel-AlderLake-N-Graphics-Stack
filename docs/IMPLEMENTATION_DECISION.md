# Native vs Linux i915: execution-path decision (2026-11-13)

## Decision
**Use the existing native RenkuOS GPU path as the primary integration baseline for the first verified GPU command.** Continue `experiment/i915-linuxkpi` as a bounded source-reuse research branch, not as a wholesale driver replacement. This is a provisional engineering decision based on existing documented evidence, **not** a benchmark or successful GPU submission.

## Why
| Capability / prerequisite | Native branch | Linux i915 experimental branch |
| --- | --- | --- |
| Physical HDMI desktop | Build 057 demonstrated | Existing display left untouched; no replacement |
| PCI, GART, memory owner | Implemented partially, physical observations/tests | Scoped native wired pages and CPU allocator; GPU DMA integration absent |
| GT forcewake / BCS reset | Physical handshake and bounded reset reported | Not integrated with GPU engine |
| PPGTT/context helpers | CPU host/native helpers exist; no live publication | No GPU context/address space wiring |
| Ring packet construction | Offline nonce/complement encoder | Original Linux ring + batch-start emitter CPU-tested |
| Actual GPU command / completion | Unproven | Unproven |
| Major blockers | DMA/cache/TLB mapping, context publication, engine service, retirement | Same GPU blockers **plus** whole-file DRM VMA, ww locking, fences, RCU and request lifecycle |

## Evidence sources
Native: `README.md`, `results/2026-11-07-reset-context-progress.md`, `GPU_FIRST_EXECUTION_PLAN.md`, `coordination/HARDWARE_TEST_RESULTS.md`.
LinuxKPI branch: `PORT_PROGRESS.md`, `PORT_COMPARISON.md`, `PORT_BLOCKERS.md`, `LINUXKPI_DEPENDENCIES.md` at `experiment/i915-linuxkpi`. That branch reports 16 x 62 successful CPU kernel-service checks on both native RenkuOS VM and physical N100, **not** GPU execution.

## Bounded integration sequence
1. Preserve working HDMI, current physical memory owner and BCS reset evidence.
2. Produce exact current deployed source/binary/patch ledger. Do not equate repository patch files with installed patches.
3. Complete one owned GPU-visible scratch/result page and translation/cache/TLB contract.
4. Complete valid BCS context/ring state, engine admission and reset-generation handling.
5. Submit a kernel-constructed nonce/complement store with an independently verified completion/retirement path and disabled-submit negative control.
6. Evaluate selective reuse of Linux `drm_mm` or emitter algorithms only where they replace concrete native gaps without introducing full Linux GEM/DRM dependencies.
7. After first verified GPU command, revisit full i915 reuse and Mesa integration based on measured effort and behavior.

## Stop conditions for branch consolidation
Do not merge LinuxKPI into native solely because its CPU-service suite passes. Require actual address-space/ABI review, clean native build, independent Terra review, and hardware evidence. No unsupported claim of native superiority: this decision minimizes known dependency scope, not measured development time.
