#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Relink the pinned Mesa compiler with native PCI-ID-only device info."""
import json
from collections import Counter
from pathlib import Path
import re
import shlex
import subprocess
import sys

TARGET = "src/intel/compiler/brw/librenku_intel_compiler.so"


def compile_override(commands, build, suffix, source, output, extra=()):
    entry = next(e for e in commands if e["file"].endswith(suffix))
    args = shlex.split(entry["command"])
    args[args.index(entry["file"])] = str(source)
    args[args.index("-o") + 1] = str(output)
    args[args.index("-MF") + 1] = str(output.with_suffix(".d"))
    args[args.index("-MQ") + 1] = str(output)
    subprocess.run(args + list(extra), cwd=build, check=True)


def main():
    if len(sys.argv) not in (7, 8):
        raise SystemExit("usage: link-native-compiler.py MESA_BUILD RUNTIME_C "
                         "NATIVE_DEVICE_INFO_C SPIRV_INCLUDE SPIRV_LIBRARY OUTDIR "
                         "[NATIVE_COMMON_C]")
    build, runtime, device, spirv_include, spirv_library, output = (
        Path(arg).resolve() for arg in sys.argv[1:7])
    common = Path(sys.argv[7]).resolve() if len(sys.argv) == 8 else None
    source = (build / "../mesa-26.3.0").resolve()
    if (source / "VERSION").read_text().strip() != "26.3.0":
        raise SystemExit("expected Mesa 26.3.0 source beside build")
    if not all(p.is_file() for p in (runtime, device, spirv_library, common)
               if p is not None):
        raise SystemExit("missing runtime, device-info, or SPIR-V library")
    if not (spirv_include / "spirv-tools/libspirv.h").is_file():
        raise SystemExit("missing SPIR-V Tools header")
    output.mkdir(parents=True, exist_ok=False)
    commands = json.loads((build / "compile_commands.json").read_text())
    runtime_object = output / "native_runtime.o"
    device_object = output / "native_device_info.o"
    compile_override(commands, build, "/native_runtime/intel_spirv_runtime.c",
                     runtime, runtime_object, ("-I" + str(spirv_include),))
    compile_override(commands, build, "/src/intel/dev/intel_device_info.c",
                     device, device_object)
    override_objects = {runtime_object, device_object}
    override_sources = {runtime, device}
    if common is not None:
        common_object = output / "native_common.o"
        compile_override(commands, build, "/src/intel/common/intel_common.c",
                         common, common_object)
        override_objects.add(common_object)
        override_sources.add(common)
    graph = (build / "build.ninja").read_text()
    start = graph.index(f"build {TARGET}: cpp_LINKER ")
    block = graph[start:].split("\n\n", 1)[0]
    link_args = shlex.split(next(line.split(" = ", 1)[1]
                                 for line in block.splitlines()
                                 if line.startswith(" LINK_ARGS = ")))
    binary = output / "librenku_intel_compiler.so"
    link_map = output / "link.map"
    subprocess.run(["c++", f"-Wl,-Map={link_map}", "-o", str(binary),
                    *map(str, sorted(override_objects)), *link_args,
                    str(spirv_library)], cwd=build, check=True)
    needed = re.findall(r"NEEDED\s+(\S+)", subprocess.run(
        ["objdump", "-p", str(binary)], check=True, text=True,
        stdout=subprocess.PIPE).stdout)
    if any("libdrm" in library for library in needed):
        raise RuntimeError("native compiler unexpectedly links libdrm")
    included = link_map.read_text().split("Allocating common symbols", 1)[0]
    objects = set(re.findall(r"(?m)^(\S+\.o)$", included))
    if any(f"LOAD {obj}" not in link_map.read_text()
           for obj in override_objects):
        raise RuntimeError("override objects not linked")
    objects.update(map(str, override_objects))
    (output / "objects.txt").write_text("\n".join(sorted(objects)) + "\n")
    dependencies = {}
    archive_objects = objects - set(map(str, override_objects))
    deps_text = subprocess.run(["ninja", "-t", "deps", *sorted(archive_objects)],
                               cwd=build, check=True, text=True,
                               stdout=subprocess.PIPE).stdout
    current = None
    for line in deps_text.splitlines():
        if line.startswith("    ") and current:
            dependencies[current].add((build / line.strip()).resolve())
        elif line.endswith(" (VALID)") or line.endswith(" (STALE)"):
            current = line.split(": #deps", 1)[0]
            dependencies[current] = set()
        else:
            current = None
    for obj in override_objects:
        contents = obj.with_suffix(".d").read_text().replace("\\\n", " ")
        dependencies[str(obj)] = {(build / p).resolve() for p in
                                  shlex.split(contents.split(":", 1)[1])}
    if set(dependencies) != objects:
        raise RuntimeError("incomplete compiler object dependencies")
    source_by_object = {entry["output"]: entry["file"] for entry in commands}
    files = {(build / source_by_object[obj]).resolve() for obj in archive_objects}
    files.update(override_sources)
    files.update(p for paths in dependencies.values() for p in paths)
    counts, sizes = Counter(), Counter()
    for path in files:
        if not path.is_file():
            raise RuntimeError("missing compiler input: " + str(path))
        group = ("mesa" if path.is_relative_to(source) else
                 "generated" if path.is_relative_to(build) else
                 "native_override" if path in override_sources else "external")
        counts[group] += 1
        sizes[group] += path.stat().st_size
    (output / "files.txt").write_text(
        "\n".join(str(path) for path in sorted(files)) + "\n")
    report = {"mesa_version": "26.3.0", "binary": str(binary),
              "binary_bytes": binary.stat().st_size,
              "linked_objects": len(objects), "runtime_libraries": needed,
              "included_files": len(files), "file_counts": dict(counts),
              "source_bytes": dict(sizes),
              "i915_drm_header_selected": any(p.name == "i915_drm.h" for p in files),
              "i915_device_info_object_selected": any(
                  "i915_intel_device_info.c.o" in obj for obj in objects),
              "i915_engine_object_selected": any(
                  "i915_intel_engine.c.o" in obj for obj in objects)}
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
