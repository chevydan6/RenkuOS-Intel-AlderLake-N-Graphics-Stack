#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
private=/boot/system/develop/headers/private
mkdir -p build
g++ -std=gnu++17 -O2 -Wall -Wextra -Werror -fPIC -ffreestanding -fno-builtin \
 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-stack-protector \
 -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1 \
 -I. -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared" \
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64" \
 -I"$private/kernel/boot/platform/bios_ia32" \
 -c bcs_engine_compile.cpp -o build/bcs_engine.o
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. bcs_engine_smoke.cpp -o build/bcs_engine_smoke
./build/bcs_engine_smoke
sha256sum BcsEngine.h IntelN100ContextStatus.h bcs_engine_compile.cpp bcs_engine_smoke.cpp \
 build-bcs-engine.sh > build/source.sha256
sha256sum build/bcs_engine.o build/bcs_engine_smoke > build/binaries.sha256
echo NATIVE_BCS_ENGINE_BUILD_PASSED
