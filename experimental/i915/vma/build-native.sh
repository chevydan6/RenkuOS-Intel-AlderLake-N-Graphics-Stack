#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir -p build
gcc --version > build/compiler.txt
uname -a > build/kernel.txt
flags=(-std=gnu11 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter
 -Wno-sign-compare -fno-strict-overflow -ffreestanding -fno-builtin -fPIC
 -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone
 -D_KERNEL_MODE=1 -DRENKU_NATIVE -Iinclude -Iupstream -Ivma)
objects=()
for source in upstream/drm/drm_vma_manager.c upstream/drm/drm_mm.c upstream/lib/rbtree.c \
 src/native_rwlocks.c src/native_alloc.c src/native_services.c src/native_diagnostics.c vma/native_test.c; do
 object="build/$(basename "${source%.c}").o"
 gcc "${flags[@]}" -c "$source" -o "$object"
 objects+=("$object")
done
gcc -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/i915_linux_vma_test.driver "${objects[@]}" \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
gcc -O2 -Wall -Wextra -Werror -Ivma vma/native_client.c -o build/linux_vma_probe
nm -u build/i915_linux_vma_test.driver > build/imports.txt
sha256sum build/i915_linux_vma_test.driver build/linux_vma_probe > build/binaries.sha256
echo 'Native whole DRM VMA build passed'
