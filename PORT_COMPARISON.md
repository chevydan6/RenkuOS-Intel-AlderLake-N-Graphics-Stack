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
