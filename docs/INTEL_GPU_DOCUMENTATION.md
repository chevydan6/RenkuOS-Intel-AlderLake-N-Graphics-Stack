<!-- SPDX-License-Identifier: MIT -->
# Intel N100 GPU documentation inventory — 2026-11-14

## What is actually available in this workspace

There is **no complete Intel N100 graphics manual set cached locally**. The PDF
inventory of the local project/download locations found zero PDFs. Existing
N100 register/packet research is mainly tied to exact Linux i915 source revisions
and the repository's native code/evidence. That source is useful primary
implementation evidence; it is not a claim that every field was checked against
an N100-specific Intel hardware manual.

The existing [N100 architecture research](../INTEL_N100_GPU_ARCHITECTURE.md) itself
records that the target PRM/stepping review remains incomplete. Preserve that
limitation in future hardware claims. See [the native handoff](../NATIVE_GPU_HANDOFF.md).

## Official Intel sources located online

- [Intel's current public graphics PRM catalog](https://www.intel.com/content/www/us/en/docs/graphics-for-linux/developer-reference/1-0/hardware-specs.html).
  Intel's [PRM support article](https://www.intel.com/content/www/us/en/support/articles/000101642/graphics.html)
  points to this collection for available public manuals. The catalog checked on
  this date has no Alder Lake entry. That observation does not establish that no
  additional documentation exists elsewhere or through Intel account access.
- [N100's official product page](https://www.intel.com/content/www/us/en/products/sku/231803/intel-processor-n100-6m-cache-up-to-3-40-ghz/specifications.html)
  identifies it as Alder Lake-N. Intel publishes an
  [N-series platform datasheet](https://cdrdv2-public.intel.com/764616/764616_004.pdf).
  A platform datasheet does not establish coverage of all command/register details.
- [Tiger Lake PRM collection](https://www.intel.com/content/www/us/en/docs/graphics-for-linux/developer-reference/1-0/tiger-lake.html):
  related reference volumes cover instruction/register encodings, structures,
  memory views/cache, command streaming, copy engine and workarounds.
- [Rocket Lake PRM collection](https://www.intel.com/content/www/us/en/docs/graphics-for-linux/developer-reference/1-0/rocket-lake.html):
  additional related-platform register/display/workaround references.

These online references have been located, not downloaded as a complete cache.
Tiger Lake, Rocket Lake and DG1 manuals must not be relabeled N100 documentation.
Check their applicability against the actual N100 IP/stepping and pinned Intel
driver logic before using a platform-specific register or workaround.

## Documentation needed for the immediate native execution path

Keep this targeted to actual GPU request execution:

1. Gen12 command-stream/context state, descriptor/ELSQ and CSB semantics.
2. GGTT/PPGTT encodings, PAT/MOCS/cache visibility and translation invalidation.
3. BCS0/copy-engine commands, completion ordering and relevant stepping workarounds.
4. Native PCI/power/forcewake/reset integration and the observed device identity.

The repository already has source-pinned research for these areas. Cross-check
the required fields in applicable Intel manuals as integration proceeds. This is
a documentation gap record, not a reason to restart general Linux compatibility
work or create an exhaustive unrelated test program.
