#!/bin/bash
# SPDX-License-Identifier: MIT
# Candidate build only. Run inside the generated candidate directory on Haiku.
set -euo pipefail
test "$(uname -s)" = Haiku
test "$(uname -m)" = x86_64
sha256sum -c candidate.sha256
source_root=${1:-/boot/home/i915-mapping-002/source}
private=/boot/system/develop/headers/private
graphics="$source_root/headers/private/graphics"
driver="$source_root/src/add-ons/kernel/drivers/graphics/intel_extreme"
common="$source_root/src/add-ons/kernel/drivers/graphics/common"
# Pin the only source being replaced to the reviewed native driver baseline.
printf '%s  %s\n' e32ce739f36af66dea0f2c2023453549be5b875c32a81c70a62dabfbb47adcb1 "$driver/intel_extreme.cpp" | sha256sum -c -
for file in "$driver"/{Jamfile,bios.cpp,driver.cpp,device.cpp,power.cpp} \
 "$common"/{Jamfile,memory_manager.c} "$source_root/src/system/kernel/util/kernel_cpp.cpp"; do
 test -f "$file"
done
# Golden source validation runs on the preparation host; checked candidate bytes
# are pinned above with candidate.sha256. Native build needs only SDK/tools.
# Use a fresh build directory; failures leave their evidence for inspection.
mkdir build
flags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
 -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -fno-use-cxa-atexit
 -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1
 -I. -I"$driver" -I"$source_root/build/config_headers"
 -I"$private" -I"$private/kernel" -I"$private/drivers" -I"$private/system" -I"$private/shared"
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64"
 -I"$private/kernel/boot/platform/bios_ia32"
 -I"$graphics" -I"$graphics/intel_extreme" -I"$graphics/common" -I"$graphics/vesa")
objects=()
for unit in bios driver device power; do
 g++ "${flags[@]}" -c "$driver/$unit.cpp" -o "build/$unit.o"
 objects+=("build/$unit.o")
done
g++ "${flags[@]}" -c intel_extreme.cpp -o build/intel_extreme.o
g++ "${flags[@]}" -c "$source_root/src/system/kernel/util/kernel_cpp.cpp" -o build/kernel_cpp.o
gcc -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC -ffreestanding -fno-builtin \
 -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1 \
 -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared" \
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64" \
 -I"$private/kernel/boot/platform/bios_ia32" -I"$source_root/build/config_headers" \
 -I"$graphics" -I"$graphics/common" -I"$graphics/radeon" \
 -DENABLE_LOGGING -DLOG_INCLUDE_STARTUP -c "$common/memory_manager.c" -o build/memory_manager.o
ar rcs build/libgraphicscommon.a build/memory_manager.o
g++ -shared -nostdlib -Wl,--no-undefined -Wl,-soname,intel_extreme \
 -Wl,-z,max-page-size=0x1000 -o build/intel_extreme \
 "${objects[@]}" build/intel_extreme.o build/kernel_cpp.o build/libgraphicscommon.a \
 -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
sha256sum build/intel_extreme > build/binaries.sha256
nm -u build/intel_extreme > build/imports.txt
nm -D --defined-only build/intel_extreme > build/exports.txt
sha256sum "$driver"/{Jamfile,bios.cpp,driver.cpp,device.cpp,power.cpp} \
 "$common"/{Jamfile,memory_manager.c} "$source_root/src/system/kernel/util/kernel_cpp.cpp" \
 > build/native-inputs.sha256
echo NATIVE_GPU_DRIVER_HANDOFF_BUILD_PASSED
