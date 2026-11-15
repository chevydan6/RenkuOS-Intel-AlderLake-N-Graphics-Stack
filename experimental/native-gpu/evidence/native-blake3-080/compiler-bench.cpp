// SPDX-License-Identifier: MIT
// Compare the same SPIR-V compiler workload with two isolated compiler links.
#include <OS.h>
#include <image.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "/boot/home/test-work/vulkan-ubo-075/vulkan/KnownSpirv.h"
#include "/boot/home/test-work/native-ubo-compiler-075/IntelSpirvRuntime.h"

int main(int argc, char** argv)
{
    const int iterations = argc > 1 ? atoi(argv[1]) : 100;
    if (iterations <= 0 || iterations > 10000)
        return 2;
    int32 cookie = 0;
    image_info image;
    while (get_next_image_info(getpid(), &cookie, &image) == B_OK)
        if (strstr(image.name, "librenku_intel_compiler.so"))
            printf("compiler=%s\n", image.name);
    uint32_t checksum = 0;
    const bigtime_t start = system_time();
    for (int i = 0; i < iterations; ++i) {
        uint8_t vertex[4096] = {}, fragment[4096] = {};
        RenkuIntelCompilePair pair = {};
        pair.abi_version = RENKU_INTEL_COMPILER_ABI;
        pair.vertex.code = vertex;
        pair.vertex.capacity = sizeof(vertex);
        pair.fragment.code = fragment;
        pair.fragment.capacity = sizeof(fragment);
        char error[256] = {};
        if (renku_intel_compile_vulkan_pair(
                RenkuKnownSpirv::Vertex,
                sizeof(RenkuKnownSpirv::Vertex) / sizeof(uint32_t),
                RenkuKnownSpirv::Fragment,
                sizeof(RenkuKnownSpirv::Fragment) / sizeof(uint32_t),
                &pair, error, sizeof(error)) != RENKU_INTEL_COMPILE_OK) {
            fprintf(stderr, "compile failed: %s\n", error);
            return 1;
        }
        checksum += vertex[0] + fragment[0] + pair.vertex.bytes
            + pair.fragment.bytes;
    }
    printf("iterations=%d elapsed_us=%lld checksum=%u\n", iterations,
        (long long)(system_time() - start), checksum);
    return 0;
}
