#!/bin/bash
# SPDX-License-Identifier: MIT
set -euo pipefail
test "$(uname -s)" = Haiku
sha256sum -c baseline-owner.sha256
owner_source=${INTEL_OWNER_SOURCE:-/boot/home/i915-mapping-002/source}
private=/boot/system/develop/headers/private
graphics="$owner_source/headers/private/graphics"
mkdir -p build
flags=(-std=gnu++17 -O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -fPIC
 -MMD
 -ffreestanding -fno-builtin -fno-exceptions -fno-rtti -fno-use-cxa-atexit
 -fno-stack-protector -fno-tree-vectorize -mgeneral-regs-only -mno-red-zone -D_KERNEL_MODE=1
 -I. -Iinclude -I"$owner_source/build/config_headers"
 -I"$private" -I"$private/kernel" -I"$private/system" -I"$private/shared"
 -I"$private/kernel/arch/x86" -I"$private/system/arch/x86_64"
 -I"$private/kernel/boot/platform/bios_ia32"
 -I"$graphics" -I"$graphics/intel_extreme" -I"$graphics/common")
if test "${NATIVE_RENDER_PROFILE_TIMING:-0}" = 1; then
 flags+=(-DNATIVE_RENDER_PROFILE_TIMING=1)
fi
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
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_lifecycle.cpp -o build/gpu_lifecycle
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. lifecycle_copy.cpp -o build/lifecycle_copy
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_copy_bench.cpp -lbe -o build/gpu_copy_bench
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. buffer_window.cpp -lbe -o build/buffer_window
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. presentation_smoke.cpp -o build/presentation_smoke
./build/presentation_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. buffer_copy_smoke.cpp -o build/buffer_copy_smoke
./build/buffer_copy_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. copy_image_smoke.cpp -o build/copy_image_smoke
./build/copy_image_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. -I"$graphics/intel_extreme" pci_power_smoke.cpp -o build/pci_power_smoke
./build/pci_power_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. csb_cursor_smoke.cpp -o build/csb_cursor_smoke
./build/csb_cursor_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. lifecycle_smoke.cpp -o build/lifecycle_smoke
./build/lifecycle_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. recovery_smoke.cpp -o build/recovery_smoke
./build/recovery_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_sample.cpp -o build/gpu_render_sample
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_vertices.cpp -o build/gpu_render_vertices
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_invalid_vertices.cpp -o build/gpu_render_invalid_vertices
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_resource_sample.cpp -o build/gpu_resource_sample
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_resource_copyout_smoke.cpp -o build/gpu_resource_copyout_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_buffer.cpp -o build/gpu_render_buffer
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_buffer_invalid.cpp -o build/gpu_render_buffer_invalid
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_batch_invalid.cpp -o build/gpu_render_batch_invalid
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_shader.cpp -o build/gpu_render_shader
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_shader_invalid.cpp -o build/gpu_render_shader_invalid
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_combined.cpp -o build/gpu_render_combined
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. gpu_render_vertex_depth.cpp -o build/gpu_render_vertex_depth
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. resource_smoke.cpp -o build/resource_smoke
./build/resource_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. resource_arena_smoke.cpp -o build/resource_arena_smoke
./build/resource_arena_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. resource_command_smoke.cpp -o build/resource_command_smoke
./build/resource_command_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. ppgtt_bind_smoke.cpp -o build/ppgtt_bind_smoke
./build/ppgtt_bind_smoke
g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. render_window.cpp -lbe -o build/render_window
for smoke in protocol_smoke.cpp render/rcs_hardware_smoke.cpp render/rcs_ownership_smoke.cpp render/rcs_context_smoke.cpp render/rcs_engine_smoke.cpp render/rcs_recovery_smoke.cpp render/rcs_reuse_smoke.cpp render/rcs_draw_smoke.cpp render/rcs_draw64_smoke.cpp render/rcs_user_vertices_smoke.cpp render/rcs_fragcoord_smoke.cpp render/rcs_resource_vertices_smoke.cpp render/rcs_resource_target_smoke.cpp render/rcs_uniform_surface_smoke.cpp render/rcs_validated_batch_smoke.cpp render/rcs_public_shader_smoke.cpp render/rcs_presentation_smoke.cpp render/rcs_timeout_smoke.cpp render/rcs_pci_admission_smoke.cpp; do
 if test -f "$smoke"; then
  target="build/$(basename "${smoke%.cpp}")"
  g++ -std=c++17 -O2 -Wall -Wextra -Werror -I. -I"$graphics/intel_extreme" "$smoke" -o "$target"
  "$target"
 fi
done
sha256sum build/intel_memory_owner build/intel_native_execution_bridge build/gpu_copy_sample > build/binaries.sha256
nm -u build/intel_memory_owner > build/imports.txt
echo NATIVE_GPU_EXECUTION_BUILD_PASSED
