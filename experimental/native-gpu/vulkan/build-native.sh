#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
root=$(dirname "$here")
test -f "$root/include/vulkan/vulkan_core.h"
mkdir -p "$here/build"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fPIC -shared -MMD \
 -MF "$here/build/libvulkan.d" \
 -I"$root/include" "$here/NativeVulkan.cpp" -o "$here/build/libvulkan.so"
g++ -std=c++17 -O2 -Wall -Wextra -Werror -MMD -MF "$here/build/api_probe.d" \
 -I"$root/include" \
 "$here/api_probe.cpp" -L"$here/build" -lvulkan \
 -Wl,-rpath,"$here/build" -o "$here/build/api_probe"
"$here/build/api_probe"
sha256sum "$here/build/libvulkan.so" "$here/build/api_probe"
