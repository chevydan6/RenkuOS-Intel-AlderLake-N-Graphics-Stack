# Archived LinuxKPI include-closure exploration

Saved when the owner chose native integration changes on November14,2026.
This branch is historical work, not the active GPU execution strategy.
The previous native-validated checkpoint is66b3981. All original118 imports
remain unchanged; two more complete GPL-2.0 timer/ratelimit headers were imported
with provenance. New declaration boundaries avoid pulling Linux VM layouts into
callers, but do not implement missing services. Namespacing resolves native/POSIX
timer-name collisions. Typed xchg and atomic_long_read are implemented.

Target-header atomic smoke passed after missing APIs were observed. Target-header
component compilation passes. All four whole original translation units still
fail; later logs expose per-CPU/radix-tree/sysfs dependencies and missing asm/stat.h.
There is no native kernel runtime or GPU execution claim for this exploration.
The working native continuation is codex/i915-native-adaptation and its
NATIVE_GPU_HANDOFF.md. The owner explicitly requested focused checks and no
exhaustive testing or general-purpose Linux compatibility detours.
