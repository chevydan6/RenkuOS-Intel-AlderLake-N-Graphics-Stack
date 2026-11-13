#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -eu
cd "$(dirname "$0")/.."
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir -p build/native
gcc --version >build/native/compiler.txt
uname -a >build/native/kernel.txt
ls /boot/system/packages/*devel*.hpkg >build/native/devel-packages.txt
python_command=
if command -v python3 >/dev/null 2>&1; then python_command=python3; fi
if [ -n "$python_command" ]; then "$python_command" verify-upstream.py; fi
# The host-prepared bundle also contains sha256sum payload verification.
kernel_flags=(-std=gnu11 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter
    -Wno-sign-compare -fno-strict-overflow -ffreestanding -fno-builtin -fPIC
    -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone
    -D_KERNEL_MODE=1 -DRENKU_NATIVE
    -Iinclude -Iupstream -Ivm
    -I/boot/system/develop/headers/os/drivers
    -I/boot/system/develop/headers/os/kernel
    -I/boot/system/develop/headers/os/support)
objects=()
for unit in upstream/drm/drm_mm.c upstream/lib/rbtree.c upstream/lib/refcount.c \
    src/native_locks.c src/native_irq_scopes.c src/task_identity.c src/time.c src/native_services.c \
    src/native_diagnostics.c vm/native_test_driver.c; do
    object="build/native/$(basename "${unit%.c}").o"
    gcc "${kernel_flags[@]}" -MMD -c "$unit" -o "$object"
    objects+=("$object")
done
gcc -shared -nostdlib -Wl,--no-undefined -Wl,-soname,i915_linuxkpi_test \
    -Wl,-z,max-page-size=0x1000 -o build/native/i915_linuxkpi_test.driver \
    "${objects[@]}" -L/boot/system/develop/lib -l:_KERNEL_ \
    /boot/system/develop/lib/haiku_version_glue.o
gcc -std=gnu11 -O2 -Wall -Wextra -Werror -Ivm \
    -I/boot/system/develop/headers/os/drivers vm/native_test_client.c \
    -o build/native/linuxkpi_native_probe
# Original Linux ring/emitter function bodies, compiled with the SAME native
# compiler and kernel SDK. Keep unresolved execution dependencies visible.
# This partial object is deliberately not installed as an execution driver.
for unit in intel_ring_slice gen8_engine_slice; do
    gcc "${kernel_flags[@]}" -MMD -c "build/$unit.c" -o "build/native/$unit.o"
done
ld -r -o build/native/i915_execution_preparation.o \
    build/native/intel_ring_slice.o build/native/gen8_engine_slice.o
nm -u build/native/i915_execution_preparation.o >build/native/execution-imports.txt
sha256sum build/native/i915_linuxkpi_test.driver build/native/linuxkpi_native_probe \
    build/native/i915_execution_preparation.o \
    >build/native/binaries.sha256
nm -u build/native/i915_linuxkpi_test.driver >build/native/imports.txt
echo 'Native build complete; module has not been installed or run by this script.'
