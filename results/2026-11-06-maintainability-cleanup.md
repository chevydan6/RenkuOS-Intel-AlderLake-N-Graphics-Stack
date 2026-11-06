# Maintainability cleanup baseline

Source baseline `gpu-hdmi-metal-056` preserves the first verified working HDMI
desktop. Cleanup branch `gpu/n100-cleanup` adds patch64, without intentionally
changing supported hardware, register values, sequencing or failure behavior.

The cleanup centralizes shared physical BAR offsets in
`AlderLakeDisplayRegisters.h`, replaces duplicated literals across fourteen
helpers, and makes pipe timing, saved state, control bits and polling names
explicit. Compound declarations and timing setup are expanded for review.
The preflight log no longer incorrectly says output is disabled when the
separate output-selection step can enable it. `MAINTENANCE.md` documents module
boundaries, ownership, cleanup retention, MBUS-local coordinates and validation.

Host sanitizer lifecycle/rollback tests and context preflight checks passed.
Native bundle `renku-057` compiled and passed artifact finalization in the
disposable VM. It was installed on the isolated RenkuTest USB and booted on the
N100; a fresh NanoKVM frame showed the normal 1920×1080 desktop. The previous
driver pair is backed up under `/boot/home/gpu-057-recovery` on that USB.

This establishes the cleanup baseline for subsequent GPU and touchpad work.
It does not assert upstream acceptance, general display-mode support, rendering
acceleration or long-term stability. Preserve the research patch history; group
it into a separate review series only after further review and validation.
