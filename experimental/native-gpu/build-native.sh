#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
owner_source=${INTEL_OWNER_SOURCE:-/boot/home/i915-mapping-002/source}
private=/boot/system/develop/headers/private
graphics="$owner_source/headers/private/graphics"
mkdir -p build
flags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
 -ffreestanding -fno-builtin
 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-stack-protector
 -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1
 -Inative-config -Ihelpers -Iinclude -Inative
 -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared"
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64"
 -I"$private/kernel/boot/platform/bios_ia32"
 -I"$graphics" -I"$graphics/intel_extreme" -I"$graphics/common")
g++ "${flags[@]}" -c src/native_page_bridge.cpp -o build/native_page_bridge.o
g++ "${flags[@]}" -c native/prepare_module.cpp -o build/prepare_module.o
g++ "${flags[@]}" -c "$owner_source/src/system/kernel/util/kernel_cpp.cpp" -o build/kernel_cpp.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/intel_native_prepare.addon build/prepare_module.o build/native_page_bridge.o build/kernel_cpp.o \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
g++ "${flags[@]}" -c native/prepare_bridge.cpp -o build/prepare_bridge.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/intel_native_prepare_bridge.driver build/prepare_bridge.o \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Inative native/prepare_client.cpp -o build/native_prepare
uname -a > build/kernel.txt
g++ --version > build/compiler.txt
sha256sum "$graphics/intel_extreme/IntelMemoryOwner.h" > build/owner-header.sha256
sha256sum build/*.addon build/*.driver build/native_prepare > build/binaries.sha256
nm -u build/intel_native_prepare.addon > build/imports.txt
echo NATIVE_GPU_PREPARATION_BUILD_PASSED
