#!/bin/bash
# SPDX-License-Identifier: MIT
# Build in an exported DMA bundle using the installed matching RenkuOS SDK.
set -eu
mkdir -p build
bsd=source/src/libs/compat/freebsd_network
private=/boot/system/develop/headers/private
flags=(-std=gnu++17 -O2 -g -Wall -fPIC -ffunction-sections -fdata-sections
 -fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-stack-protector
 -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL -D_KERNEL_MODE
 -Isource/build/config_headers -I"$bsd/compat" -I"$bsd" -I"$private" -I"$private/kernel" -I"$private/net"
 -I"$private/drivers" -I"$private/shared" -I"$private/system"
 -I"$private/kernel/boot/platform/bios_ia32" -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64")
for file in bus_dma.cpp malloc.cpp; do
 g++ "${flags[@]}" -c "$bsd/$file" -o "build/${file%.cpp}.o"
done
g++ "${flags[@]}" -Werror -Wno-builtin-declaration-mismatch -Iport/dma -c port/dma/bus_dma_bridge.cpp -o build/bus_dma_bridge.o
gcc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -DRENKU_NATIVE -D_KERNEL_MODE=1 -Iport/include -Iport/upstream -Iport/dma -c port/dma/linux_dma_api.c -o build/linux_dma_api.o
gcc -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-unused-parameter -fPIC -ffunction-sections -DRENKU_NATIVE -Iport/include -Iport/upstream -c port/build/scatterlist_slice.c -o build/scatterlist.o
cflags=(-std=gnu11 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -DRENKU_NATIVE -D_KERNEL_MODE=1 -Iport/include -Iport/upstream -Iport/dma)
for file in port/src/native_services.c port/src/native_diagnostics.c port/dma/native_test.c; do
 gcc "${cflags[@]}" -c "$file" -o "build/$(basename "${file%.c}").o"
done
g++ -shared -nostdlib -Wl,--no-undefined -Wl,--gc-sections -Wl,--version-script=port/dma/exports.map -Wl,-z,max-page-size=0x1000 -o build/i915_linux_dma_test.driver build/bus_dma.o build/malloc.o build/bus_dma_bridge.o build/linux_dma_api.o build/scatterlist.o build/native_services.o build/native_diagnostics.o build/native_test.o -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
gcc -O2 -Wall -Wextra -Werror port/dma/native_client.c -o build/linux_dma_probe
nm -u build/i915_linux_dma_test.driver
sha256sum build/i915_linux_dma_test.driver build/linux_dma_probe > build/binaries.sha256
