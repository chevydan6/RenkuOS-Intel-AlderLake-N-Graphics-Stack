#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Derive Mesa 26.3.0 PCI-ID device info without unused DRM fd queries."""
import hashlib
from pathlib import Path
import sys

PINNED_SHA256 = "dfb69cf8bfc6113bf758e13007d7e8ba2d9a20d2a4384e8c6da0be2d0b0fa468"
DROP_INCLUDES = (
    '#include "util/libdrm.h"\n',
    '#include "i915/intel_device_info.h"\n',
    '#include "xe/intel_device_info.h"\n',
    '#include "virtio/intel_virtio.h"\n',
    '#include "common/intel_gem.h"\n',
)
FD_START = "bool\nintel_get_device_info_from_fd(int fd, struct intel_device_info *devinfo, int min_ver, int max_ver)\n"
FD_END = "enum intel_wa_steppings\nintel_device_info_wa_stepping(struct intel_device_info *devinfo)\n"


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: native-device-info.py PINNED_MESA_SOURCE OUTPUT")
    original = Path(sys.argv[1]).read_bytes()
    if hashlib.sha256(original).hexdigest() != PINNED_SHA256:
        raise SystemExit("Mesa device-info source does not match pinned version")
    source = original.decode("utf-8")
    for include in DROP_INCLUDES:
        if source.count(include) != 1:
            raise SystemExit("Mesa device-info include layout changed")
        source = source.replace(include, "", 1)
    if source.count(FD_START) != 1 or source.count(FD_END) != 1:
        raise SystemExit("Mesa device-info function layout changed")
    start, end = source.index(FD_START), source.index(FD_END)
    if start >= end:
        raise SystemExit("Mesa device-info fd query boundaries reversed")
    source = source[:start] + (
        "/* Native compiler needs PCI-ID initialization; Linux DRM fd queries\n"
        " * are omitted from this derived source. */\n\n"
    ) + source[end:]
    output = Path(sys.argv[2])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(source)
    print(hashlib.sha256(output.read_bytes()).hexdigest(), output)


if __name__ == "__main__":
    main()
