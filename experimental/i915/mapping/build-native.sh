#!/bin/bash
# SPDX-License-Identifier: MIT
# Run from the prepared mapping bundle, on the target's matching installed SDK.
set -eu
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
mkdir -p build
graphics=source/headers/private/graphics
private=/boot/system/develop/headers/private
flags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
    -fno-exceptions -fno-rtti -D_KERNEL_MODE=1
    -I"$graphics" -I"$graphics/intel_extreme" -I"$graphics/common"
    -I"$private" -I"$private/kernel" -I"$private/drivers" -I"$private/shared"
    -I"$private/system" -I"$private/kernel/arch/x86"
    -I"$private/kernel/boot/platform/bios_ia32" -I"$private/system/arch/x86_64"
    -Isource/build/config_headers -ffreestanding -fno-use-cxa-atexit
    -fno-tree-vectorize -fno-strict-aliasing -fno-delete-null-pointer-checks
    -mno-red-zone -fno-omit-frame-pointer)
g++ "${flags[@]}" -c source/src/system/kernel/util/kernel_cpp.cpp -o build/kernel_cpp.o
for unit in owner gart test; do
    case "$unit" in
    owner) source_file=source/src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp;;
    gart) source_file=source/src/add-ons/kernel/busses/agp_gart/intel_gart.cpp;;
    test) source_file=mapping/native_driver.cpp;;
    esac
    g++ "${flags[@]}" -c "$source_file" -o "build/$unit.o"
    g++ -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 \
        -o "build/$unit.addon" "build/$unit.o" build/kernel_cpp.o \
        -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
done
g++ -std=c++17 -O2 -Wall -Wextra -Werror mapping/native_client.cpp -o build/i915_mapping_probe
sha256sum build/*.addon build/i915_mapping_probe > build/binaries.sha256
uname -a > build/kernel.txt
g++ --version > build/compiler.txt
echo 'Mapping candidate built; no installation or tests performed by this script.'
