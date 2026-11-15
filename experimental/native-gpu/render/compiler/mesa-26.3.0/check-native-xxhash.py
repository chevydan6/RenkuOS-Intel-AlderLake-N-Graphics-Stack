#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Compare the selected XXH32/XXH64 implementation with pinned Mesa."""
from pathlib import Path
import subprocess
import sys
import tempfile


PROGRAM = r'''
#include <stdint.h>
#include <stdio.h>
#define XXH_INLINE_ALL
#include "util/xxhash.h"

int main(void) {
    static const size_t lengths[] = {
        0, 1, 2, 3, 4, 7, 8, 15, 16, 17, 31, 32, 33, 63, 64, 65,
        127, 128, 129, 255, 256, 257, 511, 512, 1024, 4096
    };
    static const uint64_t seeds[] = {
        0, 1, 0x9e3779b9ULL, 0x0123456789abcdefULL,
        0xffffffffffffffffULL
    };
    uint8_t data[4096 + 8];
    for (size_t i = 0; i < sizeof(data); i++)
        data[i] = (uint8_t)((i * 131u + (i >> 3) * 17u) & 255u);
    for (unsigned offset = 0; offset < 8; offset++)
        for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++)
            for (size_t j = 0; j < sizeof(seeds) / sizeof(seeds[0]); j++) {
                uint64_t seed = seeds[j];
                const uint8_t *bytes = data + offset;
                printf("%u %zu %016llx %08x %016llx\n", offset,
                    lengths[i], (unsigned long long)seed,
                    XXH32(bytes, lengths[i], (uint32_t)seed),
                    (unsigned long long)XXH64(bytes, lengths[i], seed));
            }
    uint8_t single = 0xa5;
    printf("single %08x %016llx\n", XXH32(&single, 1, 7),
        (unsigned long long)XXH64(&single, 1, 7));
    printf("null-zero %08x %016llx\n", XXH32(NULL, 0, 0),
        (unsigned long long)XXH64(NULL, 0, 0));
    return 0;
}
'''


def output(source, include_root, executable):
    subprocess.run(["cc", "-std=c11", "-O2", "-Werror", "-I" + str(include_root),
                    str(source), "-o", str(executable)], check=True)
    return subprocess.check_output([str(executable)], text=True).splitlines()


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check-native-xxhash.py MESA_26_3_SOURCE")
    mesa = Path(sys.argv[1]).resolve()
    native = Path(__file__).resolve().parent / "native-headers"
    if not (mesa / "src/util/xxhash.h").is_file():
        raise SystemExit("missing pinned Mesa xxhash.h")
    if not (native / "util/xxhash.h").is_file():
        raise SystemExit("missing native util/xxhash.h")
    with tempfile.TemporaryDirectory(prefix="renku-xxhash-") as folder:
        temp = Path(folder)
        source = temp / "hashes.c"
        source.write_text(PROGRAM)
        original = output(source, mesa / "src", temp / "original")
        candidate = output(source, native, temp / "candidate")
    if len(original) != len(candidate):
        raise SystemExit("different hash case counts")
    for index, (expected, actual) in enumerate(zip(original, candidate)):
        if expected != actual:
            raise SystemExit(f"hash mismatch at case {index}: {expected} != {actual}")
    print(f"XXH32/XXH64: PASS {len(original)} Mesa-equivalent cases")


if __name__ == "__main__":
    main()
