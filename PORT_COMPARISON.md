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
