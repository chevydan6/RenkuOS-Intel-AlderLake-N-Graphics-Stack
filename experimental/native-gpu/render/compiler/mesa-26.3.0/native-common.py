#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Derive Mesa Intel common helpers without the unused DRM fd engine query."""
import hashlib
from pathlib import Path
import sys

PINNED_SHA256 = "613a579c2331fda57b3acf75c8d63756e46cb5609347d6a88a748eb711ec232d"
FUNCTION_START = "/* Updates intel_device_info fields that has dependencies on intel/common\n"
FUNCTION_END = "void\nintel_compute_engine_async_threads_limit("


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: native-common.py PINNED_MESA_SOURCE OUTPUT")
    original = Path(sys.argv[1]).read_bytes()
    if hashlib.sha256(original).hexdigest() != PINNED_SHA256:
        raise SystemExit("Mesa Intel common source does not match pinned version")
    source = original.decode("utf-8")
    for include in ("#include <stdlib.h>\n", '#include "intel_engine.h"\n'):
        if source.count(include) != 1:
            raise SystemExit("Mesa Intel common include layout changed")
        source = source.replace(include, "", 1)
    if source.count(FUNCTION_START) != 1 or source.count(FUNCTION_END) != 1:
        raise SystemExit("Mesa Intel common function layout changed")
    start, end = source.index(FUNCTION_START), source.index(FUNCTION_END)
    if start >= end:
        raise SystemExit("Mesa Intel common function boundaries reversed")
    source = source[:start] + (
        "/* Native compiler does not query DRM fd engine information. */\n\n"
    ) + source[end:]
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(source)
    print(hashlib.sha256(output.read_bytes()).hexdigest(), output)


if __name__ == "__main__":
    main()
