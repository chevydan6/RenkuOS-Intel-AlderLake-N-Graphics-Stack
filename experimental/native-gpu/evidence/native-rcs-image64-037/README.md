<!-- SPDX-License-Identifier: MIT -->
# Eleven-page RCS image construction for 64×64

`RcsDrawImage64::Build` validates eleven distinct aligned CPU pages before
writing, reuses the known-good 32×32 packet image, applies only the fifteen
Mesa 26.3.0 verified changed dwords, initializes four target pages with the
known poison value, and clears the relocated post-sync page. The new smoke
compares the complete batch, dynamic, surface, VS, and FS arrays to the
[generated 64×64 fixture](../../render/draw/RcsDrawGenerated64.h) for all four
triangle variants. It also checks invalid input leaves every page untouched.

The smoke failed to compile before the builder existed. It now passes with
`-Wall -Wextra -Werror` on the host and RenkuOS VM; see [host](host.log) and
[VM](vm.log) logs. The normal owner bundle build includes this smoke. The
existing 32×32 header has not changed in this increment. No owner module,
bridge, ioctl, or GPU submission was modified or tested here.

Next hardware step: map eleven owner pages and keep the old eight-page draw
contract intact, then add a separately versioned 16 KiB output request and
64×64 completion validation before any Vulkan image-size claim.
