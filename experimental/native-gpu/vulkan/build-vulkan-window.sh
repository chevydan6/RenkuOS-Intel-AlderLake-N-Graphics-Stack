#!/bin/bash
# SPDX-License-Identifier: MIT
# Native Haiku BWindow presenter for one compiler-backed Vulkan triangle.
set -euo pipefail
test "$(uname -s)" = Haiku
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
root=$(dirname "$here")
: "${RENKU_VULKAN_HEADERS:?set to the Khronos header directory}"
: "${RENKU_VULKAN_LIB_DIR:?set to the native libvulkan.so directory}"
test -f "$RENKU_VULKAN_HEADERS/vulkan/vulkan.h"
test -f "$RENKU_VULKAN_LIB_DIR/libvulkan.so"
test -f "$root/PresentationFrame.h"
mkdir -p "$here/build-window"
g++ -std=c++17 -O2 -Wall -Wextra -Werror \
 -MMD -MF "$here/build-window/triangle_window.d" \
 -I"$RENKU_VULKAN_HEADERS" "$here/triangle_window.cpp" \
 -L"$RENKU_VULKAN_LIB_DIR" -lvulkan -lbe \
 -Wl,-rpath,"$RENKU_VULKAN_LIB_DIR" \
 -o "$here/build-window/triangle_window"
sha256sum "$here/build-window/triangle_window"
