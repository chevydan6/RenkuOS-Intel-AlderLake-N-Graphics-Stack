#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
sha256sum -c baseline-owner.sha256
owner_source=/boot/home/i915-mapping-002/source
private=/boot/system/develop/headers/private
graphics="$owner_source/headers/private/graphics"
mkdir -p build
flags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
 -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -fno-use-cxa-atexit
 -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1
 -I. -Iinclude -I"$owner_source/build/config_headers"
 -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared"
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64"
 -I"$private/kernel/boot/platform/bios_ia32"
 -I"$graphics" -I"$graphics/intel_extreme" -I"$graphics/common")
g++ "${flags[@]}" -c IntelMemoryOwner-execution.cpp -o build/owner.o
g++ "${flags[@]}" -c src/native_page_bridge.cpp -o build/native_page_bridge.o
g++ "${flags[@]}" -c "$owner_source/src/system/kernel/util/kernel_cpp.cpp" -o build/kernel_cpp.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/intel_memory_owner build/owner.o build/native_page_bridge.o build/kernel_cpp.o \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
g++ "${flags[@]}" -c execution_bridge.cpp -o build/bridge.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
 -o build/intel_native_execution_bridge build/bridge.o \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_copy_sample.cpp -o build/gpu_copy_sample
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. buffer_copy_sample.cpp -o build/buffer_copy_sample
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_copy_bench.cpp -lbe -o build/gpu_copy_bench
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. buffer_copy_smoke.cpp -o build/buffer_copy_smoke
./build/buffer_copy_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. copy_image_smoke.cpp -o build/copy_image_smoke
./build/copy_image_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. -I"$graphics/intel_extreme" pci_power_smoke.cpp -o build/pci_power_smoke
./build/pci_power_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. csb_cursor_smoke.cpp -o build/csb_cursor_smoke
./build/csb_cursor_smoke
sha256sum build/intel_memory_owner build/intel_native_execution_bridge build/gpu_copy_sample > build/binaries.sha256
nm -u build/intel_memory_owner > build/imports.txt
echo NATIVE_GPU_EXECUTION_BUILD_PASSED
