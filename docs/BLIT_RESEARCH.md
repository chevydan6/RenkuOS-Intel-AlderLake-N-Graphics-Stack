<!-- SPDX-License-Identifier: MIT -->
# Gen12 copy experiment

`experimental/gen12/LinearCopy.h` is a host-testable C++ packet packer. It is
excluded from all driver components. It provides no MMIO, IOCTL, submission,
batch termination, register setup or GPU completion handling. Its output must
not be executed as a batch. Hardware acceleration remains unimplemented.

## Reference implementations

Linux v7.0 commit `028ef9c96e96197026887c0f092424679298aae8` provides a useful
minimal reference in
[`prepare_blit()`](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gem/selftests/i915_gem_client_blt.c).
Its linear, 32-bit branch emits ten copy dwords and configures `BLIT_CCTL`
first. The command/depth encodings and the MOCS index conversion are defined
in [`intel_gpu_commands.h`](https://github.com/torvalds/linux/blob/028ef9c96e96197026887c0f092424679298aae8/drivers/gpu/drm/i915/gt/intel_gpu_commands.h).
Both files are MIT licensed. The experiment derives the packet layout and
retains Intel attribution; the Linux engine, context and allocator are not
ported. No cache-policy index is guessed or emitted.

Mesa 26.0.7 commit `27cdb9ff9315a904b9827f9e711e68bb5e856e50`
[`gen120.xml`](https://gitlab.freedesktop.org/mesa/mesa/-/blob/27cdb9ff9315a904b9827f9e711e68bb5e856e50/src/intel/genxml/gen120.xml)
defines a 12-dword `XY_BLOCK_COPY_BLT` with per-surface MOCS fields. A search
of that pin's genxml found no `XY_FAST_COPY_BLT` definition. Consequently this
test does **not** claim agreement with a Mesa-generated fast-copy packer.
Evaluate Mesa's block-copy path before choosing the eventual driver command.

Reference content SHA-256:

| File | SHA-256 |
| --- | --- |
| Linux client blit selftest | `4de27a1ebf52ad9e8d7d538f955e36e66d7e597a5ca6766988d3d523cc569a4a` |
| Linux command definitions | `d8d14b3edefb858418eb52506f5f7598ee2cd6bd8affa659e43faaf11c85e77c` |
| Mesa Gen12 XML | `d28fa88f11a610b8ef6ae7b4addc2ce7fdf1c85f60e804a8f5c5f3339d057806` |

## Accepted experimental input

Only positive, origin-to-origin, 32-bit linear copies are modeled. GPU virtual
base addresses must be nonzero and page aligned. Pitch is positive, divisible
by four and at most 32764 bytes; coordinates fit positive signed 16-bit values.
The final accessed byte must fit each allocation. Disjoint virtual allocation
ranges are required. Failure preserves the entire output buffer.

The 39-bit address ceiling is a deliberately conservative experimental policy,
not a statement about the GPU's full virtual-address capability or physical
DMA width. Addresses here are GPU virtual addresses, never CPU pointers.
Rejecting virtual overlap cannot detect physically aliased backing.
Descriptors are trusted test metadata; this API is not a kernel security
boundary and does not pin, own or authenticate allocations.

## Verification and next gate

Host GCC with AddressSanitizer/UndefinedBehaviorSanitizer passed **60,070 checks**:
the fixed Linux linear-geometry vector, boundary rejections, unchanged failure
output, canaries and 10,000 deterministic geometries with high address bits.
These verify host arithmetic and packet fields. They do not establish hardware
legality, coherence, performance or a working RenkuOS driver.

Reproduce from the project root:

```sh
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
  -fsanitize=address,undefined tests/linear-copy-test.cpp -o build/linear-copy-test
build/linear-copy-test
```

Before any execution, resolve stolen-memory ownership, pinned GPU mappings,
physical aliases, context/address-space selection, forcewake and power domains,
MOCS initialization, required cache/TLB synchronization, batch validation,
bounded completion and reset. Start with separate kernel-owned scratch buffers
and a CPU comparison under a verified recovery window. Preserve scanout until
that path is proven. Existing legacy app_server hooks are not a modern
submission backend; desktop integration remains separate work.
