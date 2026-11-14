#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract only pinned Khronos Vulkan headers from the Mesa 26.3.0 archive."""
import argparse
import hashlib
from pathlib import Path
import tarfile

ARCHIVE_SHA256 = "91d729aa68bb89e4ec857841300bbe922bcb3e52b1f97566ca83930d3b28343a"
VULKAN = ("vulkan.h", "vulkan_core.h", "vk_platform.h")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("output", type=Path, help="new, nonexistent directory")
    args = parser.parse_args()
    data = args.archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != ARCHIVE_SHA256:
        raise ValueError("Mesa archive hash differs from pinned 26.3.0")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    records = []
    with tarfile.open(args.archive, "r:xz") as archive:
        members = [f"mesa-26.3.0/include/vulkan/{name}" for name in VULKAN]
        members += sorted(m.name for m in archive.getmembers()
                          if m.name.startswith("mesa-26.3.0/include/vk_video/")
                          and m.name.endswith(".h") and m.isfile())
        for name in members:
            source = archive.extractfile(name)
            if source is None:
                raise ValueError(f"missing header: {name}")
            relative = Path(name).relative_to("mesa-26.3.0")
            target = output / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            content = source.read()
            target.write_bytes(content)
            records.append(f"{hashlib.sha256(content).hexdigest()}  {relative}")
    (output / "SHA256SUMS").write_text("\n".join(records) + "\n")
    print(f"Extracted {len(records)} pinned Vulkan headers")

if __name__ == "__main__":
    main()
