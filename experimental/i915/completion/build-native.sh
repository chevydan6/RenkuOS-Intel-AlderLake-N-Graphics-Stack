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
 -D_KERNEL_MODE=1 -DRENKU_NATIVE -Iinclude -Iupstream -Icompletion
 -I/boot/system/develop/headers/private/system)
objects=()
for source in build/bitmap_equal_slice.c upstream/lib/find_bit.c upstream/lib/hweight.c upstream/lib/lockref.c upstream/lib/refcount.c upstream/kernel/sched/completion.c upstream/kernel/sched/swait.c \
 src/cpumask_state.c src/native_cpumask.c src/completion_sched.c src/task_identity.c src/time.c src/native_locks.c src/native_irq_scopes.c src/native_rwlocks.c src/native_preempt.c src/native_rcu.c src/native_alloc.c src/native_pages.c src/native_shmem.c src/printk_bridge.c src/native_diagnostics.c completion/native_test.c; do
 object="build/$(basename "${source%.c}").o"
 extra=()
 case "$source" in upstream/lib/find_bit.c) extra=(-Wno-error=maybe-uninitialized);; upstream/kernel/*) extra=(-include include/completion_port.h);; esac
 gcc "${flags[@]}" "${extra[@]}" -c "$source" -o "$object"
 objects+=("$object")
done
private=/boot/system/develop/headers/private
cxxflags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-stack-protector
 -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1
 -Inative-config -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared"
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64"
 -I"$private/kernel/boot/platform/bios_ia32" -Iinclude)
g++ "${cxxflags[@]}" -c src/native_page_bridge.cpp -o build/native_page_bridge.o
objects+=(build/native_page_bridge.o)
g++ "${cxxflags[@]}" -c completion/native_page_state_test.cpp -o build/native_page_state_test.o
objects+=(build/native_page_state_test.o)
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/i915_linux_completion_test.driver "${objects[@]}" \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
gcc -O2 -Wall -Wextra -Werror -pthread -Icompletion completion/native_client.c -o build/linux_completion_probe
nm -u build/i915_linux_completion_test.driver > build/imports.txt
sha256sum build/i915_linux_completion_test.driver build/linux_completion_probe > build/binaries.sha256
echo 'Native original completion/swait build passed'
