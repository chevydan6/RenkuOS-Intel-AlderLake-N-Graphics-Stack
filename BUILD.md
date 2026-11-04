# Reproducible component build

Use Python 3.12+ and Git on the workstation. Source fetches are pinned; no full
OS or Mesa build is needed for the initial two patches.

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
python3 tools/run-native.py .
```

Run `python3 tools/check-source.py build/native-fresh` on the host first.
GitHub Actions runs these source/control/manifest checks only; its green result
is not native compilation or GPU validation.

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

The source worktree is pinned RenkuOS main plus two separately attributed
patches. Haiku beta6 can be used for compilation compatibility checks because
the inspected interfaces match, but record it as Haiku-hosted validation until
the same tests run in RenkuOS. Never install the resulting kernel add-on merely
because it links against beta6.

For full Fedora image builds, inspect RenkuOS `.github/workflows/pr-gate.yml`
and pin the resolved `ghcr.io/renkuos/cross-compiler:x86_64-master` digest before
use. The repository's toolchain.pin currently names mutable branches. Use a
full-history/tagged source checkout and RenkuOS branding/revision settings,
then its configure invocation and `jam -q -j1 @nightly-anyboot`. This full image
workflow is not yet executed by this GPU project; shared toolchain and VM
provisioning are coordinated with Astra High #1.

The native runner verifies the source manifest, captures build/test output and
records kernel, compiler, RenkuOS build marker, elapsed build time, artifact
hashes and exit status in a new `validation/` directory. `--previous` compares
test status with a prior result; build time is not GPU performance. Repeated
validation uses a fresh bundle so failed-run evidence is never overwritten.
