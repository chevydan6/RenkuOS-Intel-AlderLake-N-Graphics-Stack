#!/bin/bash
# SPDX-License-Identifier: MIT
# Build a Haiku Vulkan probe pinned to an alternate native Intel compiler.
set -euo pipefail
if test "$#" -ne 4; then
 echo "usage: $0 VULKAN_BUNDLE COMPILER_DIR OUTPUT_DIR TARGET_LIBRARY_DIR" >&2
 exit 2
fi
bundle=$1
compiler=$2
output=$3
target=$4
test "$(uname -s)" = Haiku
test -f "$bundle/include/vulkan/vulkan.h"
test -f "$compiler/librenku_intel_compiler.so"
mkdir -p "$output"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared \
 -DRENKU_VULKAN_USE_INTEL_COMPILER -I"$bundle/include" \
 "$bundle/vulkan/NativeVulkan.cpp" -L"$compiler" -lrenku_intel_compiler \
 -Wl,-rpath,"$target" -o "$output/libvulkan.so"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I"$bundle/include" \
 "$bundle/vulkan/triangle_probe.cpp" -L"$output" -lvulkan -L"$compiler" \
 -Wl,-rpath,"$target" -o "$output/triangle_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
 "$bundle/vulkan/compiler_pipeline_probe.cpp" -L"$compiler" \
 -lrenku_intel_compiler -Wl,-rpath,"$target" \
 -o "$output/compiler_pipeline_probe"
LIBRARY_PATH="$output:$compiler:$bundle/lib:/boot/home/config/lib:/boot/system/lib" \
 "$output/compiler_pipeline_probe"
sha256sum "$output/libvulkan.so" "$output/triangle_probe" \
 "$output/compiler_pipeline_probe"
