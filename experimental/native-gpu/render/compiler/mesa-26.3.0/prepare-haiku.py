#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2026 Native GPU validation contributors
"""Add native shader compiler probes and runtime API to Mesa 26.3.0."""

import hashlib
from pathlib import Path
import shutil
import sys


HERE = Path(__file__).resolve().parent


def replace_once(path: Path, old: str, new: str) -> None:
    content = path.read_text()
    if new in content:
        return
    if content.count(old) != 1:
        raise SystemExit(f"unexpected Mesa input: {path}: {old!r}")
    path.write_text(content.replace(old, new, 1))


def copy_probe(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and destination.read_bytes() != source.read_bytes():
        raise SystemExit(f"refusing to replace changed probe: {destination}")
    shutil.copyfile(source, destination)
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    header = destination.parent / "source_identity.h"
    expected = f'#define NATIVE_RCS_SOURCE_SHA256 "{digest}"\n'
    if header.exists() and header.read_text() != expected:
        raise SystemExit(f"refusing to replace changed identity: {header}")
    header.write_text(expected)


def copy_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and destination.read_bytes() != source.read_bytes():
        raise SystemExit(f"refusing to replace changed runtime source: {destination}")
    shutil.copyfile(source, destination)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: prepare-haiku.py EXTRACTED_MESA_26_3_0_TREE")
    root = Path(sys.argv[1]).resolve()
    if not (root / "VERSION").exists() or (root / "VERSION").read_text().strip() != "26.3.0":
        raise SystemExit("expected an extracted Mesa 26.3.0 source tree")
    for source in (HERE / "native_rcs_shaders.c", HERE / "spirv/compile_spirv.c",
                   HERE / "intel_spirv_runtime.c", HERE / "IntelSpirvRuntime.h"):
        if not source.is_file():
            raise SystemExit(f"missing compiler probe: {source}")

    # Mesa skips these subdirectories when no Gallium or Vulkan driver is
    # selected. Build the compiler in isolation without enabling either API.
    top = root / "meson.build"
    replace_once(top, "with_any_intel = [\n", "with_any_intel = true or [\n")
    replace_once(top, "with_gfx_compute = [\n", "with_gfx_compute = true or [\n")

    brw = root / "src/intel/compiler/brw"
    meson = brw / "meson.build"
    marker = "# Native Haiku shader compiler probes."
    content = meson.read_text()
    if marker not in content:
        meson.write_text(content + "\n" + marker + "\n" + """
executable('native-rcs-shaders', 'native_rcs_shaders.c',
  include_directories: [inc_include, inc_src, inc_intel, inc_intel_compiler],
  dependencies: [idep_intel_compiler_brw, idep_intel_dev],
  install: false)

executable('native-spirv-shaders', 'native_spirv/compile_spirv.c',
  include_directories: [inc_include, inc_src, inc_intel, inc_intel_compiler],
  dependencies: [idep_intel_compiler_brw, idep_intel_dev],
  install: false)
""")
    runtime_marker = "# Native in-process Intel SPIR-V compiler."
    content = meson.read_text()
    if runtime_marker not in content:
        meson.write_text(content + "\n" + runtime_marker + "\n" + """
shared_library('renku_intel_compiler', 'native_runtime/intel_spirv_runtime.c',
  include_directories: [inc_include, inc_src, inc_intel, inc_intel_compiler],
  dependencies: [idep_intel_compiler_brw, idep_intel_dev],
  install: false)
""")
    copy_probe(HERE / "native_rcs_shaders.c", brw / "native_rcs_shaders.c")
    copy_probe(HERE / "spirv/compile_spirv.c", brw / "native_spirv/compile_spirv.c")
    copy_file(HERE / "intel_spirv_runtime.c", brw / "native_runtime/intel_spirv_runtime.c")
    copy_file(HERE / "IntelSpirvRuntime.h", brw / "native_runtime/IntelSpirvRuntime.h")
    print(f"prepared native Mesa compiler components: {root}")


if __name__ == "__main__":
    main()
