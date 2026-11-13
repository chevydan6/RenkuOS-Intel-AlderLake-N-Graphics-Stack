<!-- SPDX-License-Identifier: MIT -->
# Comparison at first compilation checkpoint
Native GPU branch already supplies tested HDMI, 64-bit GGTT changes, physical
memory/forcewake/reset evidence, and CPU context/descriptor helpers. Preserve them.

Linux reuse demonstrably supplies compilable original ring arithmetic and packet
reservation bodies. Whole-file reuse immediately encounters GEM dependencies.
The successful object uses an extracted subset and private layouts: it supports
the feasibility of selective algorithm reuse, not the superiority of full LinuxKPI.

No measured reduction in work to first GPU execution or Mesa has been established.
GT initialization, mapped owned memory, context construction, real submission and
completion remain unintegrated. Native build validation is also outstanding.
Next comparison should measure dependencies and adapter code needed to compile
the complete ring file and wire real request retirement, alongside equivalent
existing native mechanisms. File count alone is not a decision criterion.

## Complete DRM allocator result
The full Linux drm_mm algorithm and its actual tree implementation compile
against RenkuOS headers and pass allocator stress with ASan/UBSan. This is stronger
reuse evidence than extracted ring arithmetic: a complete existing dependency
works through a small compatibility surface without rewritten allocation logic.
The allocator tracks virtual ranges only. Native physical/GART ownership and
Gen12 PTE programming are still required; no performance/effort win on GPU
submission, completion or Mesa has been measured.

Whole ring reuse now demonstrably reaches GEM refcount/atomic dependencies
before request and VM integration. This supports continuing a bounded experiment,
not selecting full LinuxKPI over the existing physically tested native foundation.

## Refcount dependency result
The whole Linux refcount implementation now compiles with a small native atomic
and locking surface, retaining saturation and release/acquire algorithms. CPU
concurrency/saturation tests pass. Native kernel execution is not validated.
This advances complete-source reuse beyond the allocator but introduces concrete
OS semantics (mutex lifecycle and IRQ exclusion) absent from the initial slice.

Whole-ring reuse still brings dma_resv, wound/wait deadlock avoidance, stable task
identity and dma-fence machinery into the closure. These are now compiler-derived
dependencies rather than a speculative inventory. They remain more work than
simple typedefs or no-op wrappers. The evidence still does not select full
LinuxKPI over native execution or establish a reduced path to GPU/Mesa execution.

## Wound/wait prerequisite result
Original ww context code now compiles to a target-header ELF object and passes
CPU context tests after adding native stamp, semaphore state and scoped task
identity services. Error-pointer helpers also execute correctly on the host with
native-header error-domain preservation. This is a concrete dependency reduction.

The linker still requires real ww lock algorithms; header/context compatibility
has not solved multi-object deadlock avoidance. Fence waiting/RCU and DRM VMA
memory interfaces remain actual boundaries. Native task-scope lifetime and IRQ
semantics also need kernel runtime evidence. No reduction in measured time to
GPU execution or Mesa has been established; no path superiority is claimed.

## Direct execution-component reuse result
Two complete original Linux batch-start emitter bodies now compile against
RenkuOS headers and feed the existing original ring reservation code. Literal
packet/guard/wrap tests pass both under sanitizers and using the target-built
objects. This is command-construction reuse, not just generic compatibility code.

The selected extraction needs only a software request attribute and the ring
interface, while whole-file compilation still pulls in GEM, VMA, IDR, completion,
ww locking and RCU/VM services. This is evidence that the adaptation boundary
substantially changes the dependency burden. It does not measure first-command
hardware effort: native GT/context/mapping/submission/completion integration is
still missing, so neither full LinuxKPI nor the extracted path is declared better.

## Actual native kernel integration result

The experiment now builds and loads a real driver in RenkuOS build00081 with
the matching SDK and native GCC15.3. Original Linux allocator/tree/refcount and
ww context algorithms run against native kernel pages, mutexes, spinlocks,
threads, task identity and time. Sixteen invocations × 62 checks pass, including
concurrency, timeout/error handling and cleanup; actual unload is recorded.
The earlier absence of native runtime evidence is resolved for this subset.

Runtime exposed concrete adaptation costs: kernel threads do not retain death
entries, and device publication is asynchronous. Both failed experiments are
preserved and corrected. These are manageable OS integration details, but show
why compiling headers alone did not establish correctness.

The original execution slices also compile with the actual guest SDK into a
partial execution object. Their unresolved timeline/wait symbols, missing GPU
mappings and absent real context/submission/completion still prevent a physical
store. Native wired physical pages do not solve DMA or GPU address translation.
No CPU-service timing here measures GPU throughput, compatibility overhead,
Mesa readiness or time to first execution. Full LinuxKPI superiority remains
unproven against the native branch's existing physical GT/memory/context work.

## Physical N100 service result

The same source now builds and runs on the actual four-core N100, using its
older hrev60072+518 kernel, matching SDK and GCC13.3. Sixteen runs × 62 checks
pass again, with actual module teardown and persistent HDMI console output.
This removes uncertainty about these selected CPU adapters being VM-only or
dependent on the newer guest SDK. GPU-address ownership, GT/context setup,
submission, retirement and the broader Linux dependencies remain unresolved.
Physical CPU-service success still does not establish GPU performance overhead
or a shorter path to Mesa, and does not select the full Linux port as superior.
