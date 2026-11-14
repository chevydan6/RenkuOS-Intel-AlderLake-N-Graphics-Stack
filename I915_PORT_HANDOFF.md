<!-- SPDX-License-Identifier: MIT -->
# i915 port handoff — 2026-11-14

## Latest continuation: native store validation complete

The access blocker and pending native C++/folio/store validation described below
were resolved on November 14. The unchanged version 9 candidate passed 200,824
physical native checks, both diagnostic unloads, all 7,111 source hash checks
and production component postchecks. See
[physical-store-001](experimental/i915/evidence/physical-store-001/README.md).
The full original driver still does not compile or execute GPU commands.

**Owner clarification:** SSH is authorized for development, builds and tests,
as well as transfers and collection. Keep progress visible through NanoKVM;
the earlier keyboard-only execution restriction was an agent interpretation.
SSH currently runs on port 2222 for this boot; credentials remain private.
The owner also authorized recovery power cycles through the paired Govee outlet
when the laptop hangs, without asking again. Preserve NanoKVM power, boot media
and the exclusive physical-test lease.

Next: connect native-backed storage to genuine Linux file/address_space/shmem
interfaces and resolve the process-mm/task compilation boundary. Historical
checkpoints and the validation recipe below retain their original context.

## Objective and current branch

Continue porting the original Linux i915 driver to RenkuOS. The goal is a working
driver with real GPU submission and completion. Do not replace it with another
driver or declare success from compatibility-library tests.

- Repository: https://github.com/chevydan6/haiku-intel-gpu
- Branch: `experiment/i915-linuxkpi`; do not push directly to `main`.
- Latest implementation: `57d16db` (persistent backing and pin ownership).
- Latest implementation evidence: `cf88fa7439df10f6c64604e966bd1eeee0cd482e`.
- Everything through that checkpoint is committed and pushed. This handoff adds
  documentation; no implementation changes are pending in the old worktree.

**The full original i915 driver still does not compile. No GPU command execution,
accelerated rendering, Mesa or Vulkan result is claimed.**

## What is implemented and verified

The experiment preserves 118 byte-exact Linux v6.6 imports, with Git-blob and
SHA256 provenance. Native adapters run original completion/swait, DRM VMA and
allocator algorithms. Scoped atomic, lock, IRQ/preemption, RCU, sequence-lock,
DMA API and owned-page work has evidence under `experimental/i915/evidence/`.
These scoped results do not establish full Linux subsystem compatibility.

The last actual physical native checkpoint is
[physical-pages-001](experimental/i915/evidence/physical-pages-001/README.md):
191,776 passing checks, real wired allocations/deletions and actual diagnostic
unloads on the Intel N100. This uses diagnostic report **version 7**.

Two later additions are **host-tested, awaiting native validation**:

1. [Folio heads, original local maps and batch reference release](experimental/i915/evidence/folio-host-001/README.md):
   47,416 host sanitizer checks; version 8.
2. [Persistent backing stores and native pin ownership](experimental/i915/evidence/store-host-001/README.md):
   49,504 host ASan/UBSan checks, including four new mutation controls; version 9.
   Header tests pass 94 include combinations, 18 expected type/configuration
   rejections and four unsupported-operation link failures. The C adapters,
   native C tests and userspace client compile with the pinned native headers.

The latest store keeps backing independently of folio pins. Its C++ bridge uses
native `B_NO_LOCK` anonymous areas, writable `lock_memory_etc` pins, actual
physical-extent queries and matching `unlock_memory_etc`. Final folio release
queues unpin to the joined release worker. Concurrent first pins share one
canonical head and release redundant native pins. Live, queued and in-flight
pins prevent store destruction; failed deletion preserves the owner for retry.

The host backend models pins and physical identity using real RAM and threads.
It does **not** prove native faults, reclaim, paging, swap or DMA. The C++ cross
probe failed because the host setup lacks the matching target `<new>` header.
The new native C++ bridge still needs native compilation, export/link checks
and runtime testing. Do not invent replacement headers to hide that failure.

## Current blocker and hardware state

The previous session stopped on native access. NanoKVM keyboard writes repeatedly
returned `timeout after 50ms: write /dev/hidg0: i/o timeout`; recovery SSH was
unavailable. A physical **RenkuOS restart** was requested, not confirmed.

On resuming this session, the KVM screenshot still showed the old successful
owned-page Terminal. SSH to the known native address now returned **No route to
host**. Recheck power, boot state and the current IP; do not assume the old IP is
still correct. No new code was transferred, built or installed on RenkuOS.

- Native N100: last known `192.168.1.114`, GPU `8086:46d1`.
- NanoKVM: `192.168.1.210`.
- Last native kernel: `hrev60072+518`, matching SDK 518, GCC/G++ 13.3, four CPUs.
- Native test SSH: port `2222`, user `user` (UID 0); it was visibly stopped at
  the last successful native checkpoint. Default SSH port 22 was not usable
  with the existing authorized key during recovery.
- Boot media must remain `/root/renku-gpu-test-004/disk.img`, `ro=0`, `cdrom=0`.
  Those values were rechecked during this handoff.
- **Never call `/api/hid/reset` or rebind the USB gadget:** that can reset the
  mounted boot medium. Prior remote GPIO reset did not produce a verified reboot.
- Shared physical-test lock on NanoKVM:
  `/root/power-recovery-20261105/physical-test.lock`. Acquire an exclusive bounded
  lease before native input/testing; do not interfere with another holder.
- Latest access-check lease `/root/i915-store-access-20261114-002` was released;
  its process exited. No build/test job was started or remains queued.

## Starting on another computer

```sh
git clone --branch experiment/i915-linuxkpi \
  https://github.com/chevydan6/haiku-intel-gpu.git
cd haiku-intel-gpu
python3 experimental/i915/verify-upstream.py
```

Read applicable `AGENTS.md` instructions and repository policy. New glue is MIT;
preserve upstream licenses and byte-exact imports. Sign off commits and disclose
machine assistance. Imported code requires adjacent `Ported-from:` and
`Ported-from-license:` trailers. Keep experimental changes on this branch.

Recreate source checkouts on the new computer. The old `/tmp/i915-linux-source`
is gone and `/home/dan/renkuos-i915-source/.git` is currently invalid; do not rely
on those old paths. Place these checkouts alongside the GPU repository:

```sh
git clone --filter=blob:none --no-checkout \
  https://github.com/torvalds/linux.git ../linux-v6.6
git -C ../linux-v6.6 sparse-checkout init --cone
git -C ../linux-v6.6 sparse-checkout set include drivers/gpu/drm/i915
git -C ../linux-v6.6 checkout --detach ffc253263a1375a65fa6c9f62a893e9767fbebfa

git clone --filter=blob:none --no-checkout \
  https://github.com/RenkuOS/Source.git ../renku-source
git -C ../renku-source sparse-checkout init --cone
git -C ../renku-source sparse-checkout set headers build/config_headers
git -C ../renku-source checkout --detach 4799bb5a188c4bb6bbef967287640cc21a061a0d
```

The original computer now also has verified persistent replacements at
`/home/dan/i915-sources/linux-v6.6` and
`/home/dan/i915-sources/renku-source`. A fresh bundle generated from those trees
reproduced all 7,111 prepared store002 payload files byte-for-byte, including
6,783 reference files. The target-header C build passed from the restored paths.
See the [reproduction record](experimental/i915/evidence/store-host-001/fresh-source-reproduction.json).
This restores the source setup; it does not validate the new code on RenkuOS.

Access credentials are **not in this repository**. Transfer them privately if
needed. The old computer's `/home/dan/i915-vm/private/` contains the existing SSH
key, known-host records and NanoKVM connection configuration. Never commit that
directory, tokens, passwords or private keys. The nonsecret local convenience
scripts under `/home/dan/i915-vm/` are optional; the NanoKVM UI and standard SSH/SCP
can perform the same authorized access/transfer actions.

## Native validation recipe (completed at physical-store-001)

1. Recover RenkuOS input/access, inspect the actual screen and acquire a fresh
   exclusive test lease. Preserve the existing HDMI/display, owner and GART
   baseline. Do not replace the graphics driver for these CPU/VM diagnostics.
2. Keep development observable in RenkuOS Terminal through NanoKVM. SSH may
   execute development, builds and tests; displaying a live log in fullscreen
   native Terminal satisfies the owner's request to watch development.
3. If needed, start the existing test SSH service visibly in that Terminal:

   ```sh
   /bin/sshd -f /boot/home/i915-linuxkpi-access-20261113/sshd_config
   ```

4. Generate a fresh complete bundle on the new computer, from the GPU repo root:

   ```sh
   python3 experimental/i915/completion/prepare-bundle.py \
     --linux-source ../linux-v6.6 --renku-source ../renku-source \
     --output ../i915-native-bundle
   ```

   This exports the original configuration headers and full compiler references;
   do not reconstruct private native VM structures or generated Linux VM bounds.
   Transfer `payload.tar.gz` to RenkuOS, extract in a fresh directory, and verify
   `payload.sha256`. A full freshly generated bundle needs no reference symlink.
   Do not overwrite an existing test directory without inspecting it first.
5. Use the committed [visible native runner](experimental/i915/evidence/store-host-001/visible-run.sh).
   Its directory is `/boot/home/i915-store-001`; adjust that one path if using a
   different fresh directory. Execute it visibly in native Terminal. It checks
   sources before/after, builds with native GCC/G++, runs four completion tests
   and four VMA tests, preserves binaries and attempts four complete originals:
   GTT GEM, physical GEM, shmem GEM and ring.
6. Require zero functional failures, correct source hashes and actual diagnostic
   unload. Version 9 has a 112-byte report and expects seven owned allocations/
   frees, two page workers/512 iterations, one store create/free and two store
   workers/256 iterations per completion run. Do not invent a native check count.
   Whole-source failures are recorded separately; they are not a full-driver pass.
7. Collect build logs, source checks, binary hashes, runtime reports, unload
   records and full-source compiler errors. Fix actual native failures and retest.
   Native paging/swap pressure needs additional evidence beyond data retention.
8. Release the physical-test lease after cleanup. The owner requested continued
   SSH access; leave the service available when needed. To stop an obsolete
   temporary listener after arranging access, its original recipe was:

   ```sh
   kill $(cat /boot/home/i915-linuxkpi-access-20261113/sshd.pid)
   ```

   Verify diagnostic removal/unload and unchanged original graphics/media state.

## Work after native memory validation

Connect store ownership to real Linux `struct file`/`address_space` and shmem APIs.
The internal provider is not yet wired into `shmem_file_setup` or i915 GEM. Required
work includes GFP/reclaim constraints, pin/unpin lifetimes, dirty/accessed state,
writeback, truncate, user mappings and file/mapping ownership. Do not supply
successful no-ops for missing cache or reclaim behavior.

Complete physical/shmem GEM cross builds still reach
`shmem_fs.h -> swap.h -> mmzone.h`, deliberately rejected generated VM bounds and
missing `asm/page.h`. Ring/GTT have real process-mm/task dependencies in
`sched/mm.h`. Resolve these against native services without guessed Linux layouts.
After full source closure, continue PCI attachment, actual GT/context/mapping,
DMA coherency and submission/retirement integration. CPU tests are prerequisites;
the goal remains the full i915 port and verified GPU execution.

## Evidence and old-computer artifacts

- [Detailed implementation contracts and test recipes](experimental/i915/completion/README.md)
- [Latest public store evidence](experimental/i915/evidence/store-host-001/README.md)
- [Pinned native API source contracts](experimental/i915/evidence/store-host-001/native-api-contract.json)
- [Prepared payload hashes and report metadata](experimental/i915/evidence/store-host-001/ready-native-002.json)
- [Historical port progress](PORT_PROGRESS.md), [blockers](PORT_BLOCKERS.md),
  [dependency inventory](LINUXKPI_DEPENDENCIES.md). Later dated entries supersede
  older entries; the latest additions remain native-pending.

Optional artifacts still on the old computer:

- `/home/dan/i915-vm/store-port-001/native-bundle-002/payload.tar.gz`:
  SHA256 `9ac4a87a60c74a5bf9167a0f106b094e86d4819e1743b40774359c7427e6d9bb`.
- `/home/dan/i915-vm/store-port-001/runtime-payload-002.tar.gz`:
  SHA256 `ebdb7d7966ca98e2451fc4266b794fc957e0c7c09dfe0350ba10578a94a6ae06`.
  This smaller archive requires the existing, byte-verified reference tree at
  `/boot/home/i915-pages-001/reference`; prefer a full bundle on the new computer.
- `/home/dan/i915-vm/store-port-001/`: raw host logs, probes and prepared runner.
- `/home/dan/i915-vm/PHYSICAL_HANDOFF.md` and the Desktop handoff: long historical
  session notes. They are optional context, not required to understand this file.

Keep native-pending claims explicit when updating GitHub. Do not mark the full
port complete until the actual driver and required GPU behavior are verified.

