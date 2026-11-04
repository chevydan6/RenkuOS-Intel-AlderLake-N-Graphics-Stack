# Reproducible component build

Use Python 3.12+ and Git on the workstation. Source fetches are pinned; no full
OS or Mesa build is needed for the initial two patches.

```sh
python3 tools/prepare.py --output build/native-fresh
```

An existing Haiku clone can be supplied with `--source /path/to/haiku`. Export
always uses the pinned commit, not that checkout's working files. The output
must not exist. The bundle contains source notices, patches applied to the
original source, tests, a Makefile and a SHA-256 payload manifest.

Transfer the bundle to a new **user directory** in an x86_64 Haiku development
guest, with development headers/library matching the running kernel. Under the
shared Fedora build lock, run:

```sh
make -j1 test
```

This builds the Intel GART kernel add-on and complete Intel accelerant, runs
source table checks and real linked-accelerant hook tests, then validates ELF
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
