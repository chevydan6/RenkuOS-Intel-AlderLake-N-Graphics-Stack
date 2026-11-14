<!-- SPDX-License-Identifier: MIT -->
# Seven native GPU copies in one boot — 2026-11-14

Starting commit: `7f32216`, branch `codex/i915-native-adaptation`.
Physical Intel N100 / 8086:46d1 revision0, RenkuOS hrev60072+518.

The matching native owner was built in `/boot/home/native-repeat-003`, installed,
and loaded by a controlled reboot. Seven separate `build/gpu_copy_sample`
invocations then passed in that single boot, stopping the loop on any failure.
[Runtime](runtime.log) records sequences1–7, each with matching nonce/complement,
ordered breadcrumb, both CSB transitions, all1024dwords and both data hashes.
CSB tails are2,4,6,8,10,0,2: the sixth request wraps and the seventh reuses slots1/2.
All requests report ten private pages and the same retained mapping set; no
per-request allocation occurs. EIR/fault remain0, engine idle is0x200, forcewake
release completes at stage6, and [app_server](app-server.log) remains PID127.

The [native build](build.log) passed kernel compilation/export linking, image,
PCI-anchor and captured-CSB checks. Focused host publication smoke also passed:
renew a retired publication deadline and publish again; reject rearming a failed
LOAD. Image smoke checks that a second sequence preserves CSB entries/cursor.
No timeout or fault was injected on physical hardware; fail-stop behavior is
implemented by leaving the admission gate closed on every unsuccessful path.

Retirement requires matching activation and idle switch-away for the current
tag, exact post-flush sequence/nonce/complement, CPU visibility, all copied data,
engine idle and live owner/DMA/error checks. Successful power release and final
owner/DMA revalidation are required before the admission gate reopens. Context,
ring, source/destination and result can then be rewritten. FORCE_RESTORE remains
in Intel's descriptor. Device reset/cache/TLB/engine setup occur only on the first
request; subsequent requests preserve the HWSP and software CSB cursor. The next
two CSB slots are poisoned before publication so old wraparound entries cannot
be accepted before the new GPU writes become visible. The software head follows
pinned Linux v6.6 `process_csb`; no per-request hardware cursor reset is used.

Unknown completion, failed release or other uncertainty retains all backing and
keeps submissions blocked. Reboot remains recovery. Even on success this milestone
retains and reuses the session's allocations; it does not unmap/free GPU backing.
The monotonic32-bit sequence stops before wrapping and skips the poison value;
hardware tags cycle through1..2046 only after retirement.

[Installed owner](installed.sha256):
`79b4a82f60a1dbe8302b21eab3899f1a9753435bda691449a7972890cc9b0261`.
The installed bridge remains the execution005 bridge (ABI unchanged).
[Source verification](source-check.log), [source hashes](payload.sha256),
[unchanged display/observer components](unchanged-after.log) are retained.
The physical lease `native-repeat-20261114-001` was released after postchecks.
HDMI build057 and NanoKVM boot media were preserved.

Full native source/objects/binaries/log archive remains locally at
`/home/dan/projects/native-repeat-evidence-003.tar.gz` and on target under
`/boot/home/native-repeat-evidence-003.tar.gz`, SHA256
`b433e18d8ea26cca13fee4ff57b0ef4a1d7562093a1862dfc603211e1732da5f`.
Native wall time is incorrect; hashes and runtime records identify this trial.
