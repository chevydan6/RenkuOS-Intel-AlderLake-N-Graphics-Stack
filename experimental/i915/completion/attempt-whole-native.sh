#!/bin/bash
# SPDX-License-Identifier: MIT
# Preserve actual native compiler exits; failed source files are not modules.
set -euo pipefail
mkdir -p whole-build
flags=(-std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare
 -fno-strict-overflow -ffreestanding -fno-builtin -fno-stack-protector
 -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -DRENKU_NATIVE -D_KERNEL_MODE=1
 -Iinclude -Iupstream -Ireference/drivers/gpu/drm/i915 -Ireference/drivers/gpu/drm/i915/gem
 -Ireference/drivers/gpu/drm/i915/gt -Ireference/include -Ireference/include/uapi -fmax-errors=12)
failed=0
for source in gem/i915_gem_gtt.c gem/i915_gem_phys.c gem/i915_gem_shmem.c gt/intel_ring.c; do
 name=$(basename "$source" .c)
 test ! -e "whole-build/$name.log"
 rm -f "whole-build/$name.o"
 set +e
 gcc "${flags[@]}" -c "upstream/$source" -o "whole-build/$name.o" > "whole-build/$name.log" 2>&1
 status=$?
 set -e
 echo "$status" > "whole-build/$name.exit"
 echo "NATIVE_WHOLE_SOURCE $source compiler_exit=$status"
 if [ "$status" != 0 ]; then failed=1; fi
done
exit "$failed"
