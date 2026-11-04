# Reproducible component build

Use Python 3.12+ and Git on the workstation. Source fetches are pinned; no full
OS or Mesa build is needed for the initial component patches.

```sh
python3 tools/prepare.py --output build/native-fresh
```

An existing RenkuOS clone can be supplied with `--source /path/to/renkuos`. Export
always uses the pinned commit, not that checkout's working files. The output
must not exist. The bundle contains source notices, patches applied to the
original source, tests, a Makefile and a SHA-256 payload manifest.

Transfer the bundle to a new **user directory** in an x86_64 RenkuOS development
guest, with development headers/library matching the running kernel. Under the
shared Fedora build lock, run:

```sh
bash tools/run-native.sh
```

Run `python3 tools/check-source.py build/native-fresh` on the host first.
GitHub Actions runs source/control/manifest checks and the GGTT arithmetic tests
with address/undefined-behavior sanitizers; its green result is not native
driver compilation or GPU validation.

This builds the Intel GART kernel add-on and complete Intel accelerant, runs
source table checks and real linked-accelerant hook tests, reproduces the
original failures using an unpatched linked control, then validates ELF
architecture, entry points and kernel dependency `_KERNEL_`. It does not install
a driver or call accelerant initialization. New C++ tests use GNU C++17; native
GCC 13.3.0 was verified. Kernel flags disable exceptions, RTTI, red zone and
vectorization. Original C common sources remain C.

The component Makefile follows relevant Jam target source lists. It is not a
full official Jam image-build validation or proof of ABI compatibility with a
different kernel. Record source commit, compiler/kernel version, patch hashes,
commands, warnings, exit status, binary hashes and test results together.

Build logs and binaries remain under ignored directories. Use one compile job
on the 4 GiB target; request the authoritative shared build lock first.

The source worktree is pinned RenkuOS main plus separately attributed patches.
Patches 1–4 passed in RenkuOS hrev60072+518 with the matching development package;
later patches must pass a new native run before inheriting that result. Haiku
beta6 remains a separate compatibility check. Never install a kernel add-on
merely because it links against beta6 or the RenkuOS SDK.

For full Fedora image builds, inspect RenkuOS `.github/workflows/pr-gate.yml`
and pin the resolved `ghcr.io/renkuos/cross-compiler:x86_64-master` digest before
use. The repository's toolchain.pin currently names mutable branches. Use a
full-history/tagged source checkout and RenkuOS branding/revision settings,
then its configure invocation and `jam -q -j1 @nightly-anyboot`. This full image
workflow is not yet executed by this GPU project; shared toolchain and VM
provisioning are coordinated with Astra High #1.

The shell runner needs Bash, Make, GCC/binutils, sha256sum and tar; Python is
not required inside RenkuOS. It records the kernel/compiler/development package,
verifies every payload hash, builds and runs the native tests and original hook
control. Copy its `validation/` directory back, then run on the workstation:

```sh
python3 tools/finalize-native.py build/native-fresh /path/to/validation
```

The finalizer verifies source/control/manifest checks and extracted ELF artifacts,
then records binary hashes and the native exit status. A standalone guest run is
incomplete until host finalization passes. The older Python native runner remains
available for guests containing Python; its `--previous` compares prior test
status. Repeated validation uses fresh bundles and preserves failed-run evidence.

`tools/renku-vm-test.py` runs on Fedora within the authoritative host-config,
haiku-vm and build locks. It flattens a stopped disk into a new private directory,
boots an emulated guest, runs the component workflow, collects logs and shuts down
the guest. It never installs GPU modules or passes through PCI devices. Its SSH
configuration and verified host key remain private. Logs distinguish clean guest
shutdown from forced cleanup; never reuse a disk marked for discard.
