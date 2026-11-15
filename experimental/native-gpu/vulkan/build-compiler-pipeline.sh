#!/bin/bash
# SPDX-License-Identifier: MIT
# Compile-only integration test for a Haiku VM without N100 PCI passthrough.
set -euo pipefail
test "$(uname -s)" = Haiku
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
root=$(dirname "$here")
: "${RENKU_VULKAN_HEADERS:?set to a directory containing vulkan/vulkan.h}"
: "${RENKU_INTEL_COMPILER_LIB_DIR:?set to the directory containing librenku_intel_compiler.so}"
test -f "$RENKU_VULKAN_HEADERS/vulkan/vulkan.h"
test -f "$RENKU_INTEL_COMPILER_LIB_DIR/librenku_intel_compiler.so"
test -f "$root/render/draw/RcsDrawGenerated.h"
mkdir -p "$here/build-compiler"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared \
 -MMD -MF "$here/build-compiler/libvulkan.d" \
 -DRENKU_VULKAN_USE_INTEL_COMPILER -I"$RENKU_VULKAN_HEADERS" \
 "$here/NativeVulkan.cpp" -L"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -lrenku_intel_compiler -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/libvulkan.so"
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
 -MMD -MF "$here/build-compiler/compiler_pipeline_probe.d" \
 "$here/compiler_pipeline_probe.cpp" -L"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -lrenku_intel_compiler -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/compiler_pipeline_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -DRENKU_VULKAN_USE_INTEL_COMPILER \
 -MMD -MF "$here/build-compiler/compiler_pipeline_api_probe.d" \
 -I"$RENKU_VULKAN_HEADERS" "$here/compiler_pipeline_api_probe.cpp" \
 -L"$RENKU_INTEL_COMPILER_LIB_DIR" -lrenku_intel_compiler \
 -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/compiler_pipeline_api_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -DRENKU_VULKAN_USE_INTEL_COMPILER \
 -MMD -MF "$here/build-compiler/fence_probe.d" \
 -I"$RENKU_VULKAN_HEADERS" "$here/fence_probe.cpp" \
 -L"$RENKU_INTEL_COMPILER_LIB_DIR" -lrenku_intel_compiler \
 -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/fence_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -DRENKU_VULKAN_USE_INTEL_COMPILER \
 -MMD -MF "$here/build-compiler/semaphore_probe.d" \
 -I"$RENKU_VULKAN_HEADERS" "$here/semaphore_probe.cpp" \
 -L"$RENKU_INTEL_COMPILER_LIB_DIR" -lrenku_intel_compiler \
 -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/semaphore_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -DRENKU_VULKAN_USE_INTEL_COMPILER \
 -MMD -MF "$here/build-compiler/command_reset_probe.d" \
 -I"$RENKU_VULKAN_HEADERS" "$here/command_reset_probe.cpp" \
 -L"$RENKU_INTEL_COMPILER_LIB_DIR" -lrenku_intel_compiler \
 -Wl,-rpath,"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -o "$here/build-compiler/command_reset_probe"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I"$RENKU_VULKAN_HEADERS" \
 -MMD -MF "$here/build-compiler/triangle_probe.d" \
 "$here/triangle_probe.cpp" -L"$here/build-compiler" -lvulkan \
 -L"$RENKU_INTEL_COMPILER_LIB_DIR" \
 -Wl,-rpath,"$here/build-compiler" -o "$here/build-compiler/triangle_probe"
LIBRARY_PATH="$root/lib:$RENKU_INTEL_COMPILER_LIB_DIR:/boot/home/config/lib:/boot/system/lib" \
 "$here/build-compiler/compiler_pipeline_probe"
LIBRARY_PATH="$root/lib:$RENKU_INTEL_COMPILER_LIB_DIR:/boot/home/config/lib:/boot/system/lib" \
 "$here/build-compiler/compiler_pipeline_api_probe"
LIBRARY_PATH="$root/lib:$RENKU_INTEL_COMPILER_LIB_DIR:/boot/home/config/lib:/boot/system/lib" \
 "$here/build-compiler/fence_probe"
LIBRARY_PATH="$root/lib:$RENKU_INTEL_COMPILER_LIB_DIR:/boot/home/config/lib:/boot/system/lib" \
 "$here/build-compiler/semaphore_probe"
LIBRARY_PATH="$root/lib:$RENKU_INTEL_COMPILER_LIB_DIR:/boot/home/config/lib:/boot/system/lib" \
 "$here/build-compiler/command_reset_probe"
sha256sum "$here/build-compiler/libvulkan.so" \
 "$here/build-compiler/compiler_pipeline_probe" \
 "$here/build-compiler/compiler_pipeline_api_probe" \
 "$here/build-compiler/fence_probe" \
 "$here/build-compiler/semaphore_probe" \
 "$here/build-compiler/command_reset_probe" \
 "$here/build-compiler/triangle_probe"
