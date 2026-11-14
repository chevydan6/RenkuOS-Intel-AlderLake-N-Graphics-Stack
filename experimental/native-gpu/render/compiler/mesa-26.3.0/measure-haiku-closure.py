#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright 2026 Native GPU validation contributors
"""Measure source files selected by the native Haiku SPIR-V compiler link."""

import json
from collections import Counter
from pathlib import Path
import re
import shlex
import subprocess
import sys


DEFAULT_TARGET = "src/intel/compiler/brw/native-spirv-shaders"


def main() -> None:
    if len(sys.argv) not in (3, 4, 7):
        raise SystemExit("usage: measure-haiku-closure.py MESA_BUILD NEW_OUTPUT_DIR "
                         "[LINK_TARGET [RUNTIME_SOURCE SPIRV_INCLUDE SPIRV_LIBRARY]]")
    build = Path(sys.argv[1]).resolve()
    target = sys.argv[3] if len(sys.argv) >= 4 else DEFAULT_TARGET
    override = len(sys.argv) == 7
    source = (build / "../mesa-26.3.0").resolve()
    if (source / "VERSION").read_text().strip() != "26.3.0":
        raise SystemExit("expected Mesa 26.3.0 source beside the build")
    output = Path(sys.argv[2]).resolve()
    output.mkdir(parents=True, exist_ok=False)

    graph = (build / "build.ninja").read_text()
    start = graph.index(f"build {target}: cpp_LINKER ")
    block = graph[start:].split("\n\n", 1)[0]
    object_file = block.splitlines()[0].split(" cpp_LINKER ", 1)[1].split(" | ", 1)[0]
    link_args = shlex.split(next(line.split(" = ", 1)[1] for line in block.splitlines()
                                if line.startswith(" LINK_ARGS = ")))
    commands = json.loads((build / "compile_commands.json").read_text())
    source_by_object = {entry["output"]: entry["file"] for entry in commands}
    if override:
        runtime_source = Path(sys.argv[4]).resolve()
        spirv_include = Path(sys.argv[5]).resolve()
        spirv_library = Path(sys.argv[6]).resolve()
        if not (runtime_source.is_file() and
                (spirv_include / "spirv-tools/libspirv.h").is_file() and
                spirv_library.is_file()):
            raise RuntimeError("missing current runtime source or SPIRV-Tools input")
        entry = next(item for item in commands if item["output"] == object_file)
        compile_args = shlex.split(entry["command"])
        compile_args[compile_args.index(entry["file"])] = str(runtime_source)
        new_object = output / "current_runtime.o"
        depfile = output / "current_runtime.d"
        compile_args[compile_args.index("-o") + 1] = str(new_object)
        compile_args += ["-I" + str(spirv_include), "-MMD", "-MF", str(depfile)]
        subprocess.run(compile_args, cwd=build, check=True)
        object_file = str(new_object)
        source_by_object[object_file] = str(runtime_source)
        link_args.append(str(spirv_library))
    binary = output / Path(target).name
    link_map = output / "link.map"
    subprocess.run(["c++", f"-Wl,-Map={link_map}", "-o", str(binary),
                    object_file, *link_args], cwd=build, check=True)

    # Haiku's GNU ld lists each extracted .a.p object at the start of its map.
    included = link_map.read_text().split("Allocating common symbols", 1)[0]
    objects = set(re.findall(r"(?m)^(\S+\.o)$", included)) | {object_file}
    missing = objects - source_by_object.keys()
    if missing:
        raise RuntimeError(f"missing compile commands: {sorted(missing)[:5]}")

    ninja_objects = objects - ({object_file} if override else set())
    deps_text = subprocess.run(["ninja", "-t", "deps", *sorted(ninja_objects)],
                               cwd=build, check=True, text=True,
                               stdout=subprocess.PIPE).stdout
    dependencies = {}
    current = None
    for line in deps_text.splitlines():
        if line.startswith("    ") and current:
            dependencies[current].add((build / line.strip()).resolve())
        elif line.endswith(" (VALID)") or line.endswith(" (STALE)"):
            current = line.split(": #deps", 1)[0]
            dependencies[current] = set()
        else:
            current = None
    if override:
        contents = depfile.read_text().replace("\\\n", " ").split(":", 1)[1]
        dependencies[object_file] = {(build / path).resolve()
                                     for path in shlex.split(contents)}
    missing = objects - dependencies.keys()
    if missing:
        raise RuntimeError(f"missing Ninja dependencies: {sorted(missing)[:5]}")

    files = {(build / source_by_object[obj]).resolve() for obj in objects}
    files |= set().union(*(dependencies[obj] for obj in objects))
    counts, sizes = Counter(), Counter()
    for file in files:
        if not file.is_file():
            raise RuntimeError(f"missing linked input: {file}")
        if override and file.is_relative_to(runtime_source.parent):
            group = "project_runtime"
        elif file.is_relative_to(source / "src/intel/compiler/brw/native_spirv"):
            group = "project_probe"
        elif file.is_relative_to(source / "src/intel/compiler/brw/native_runtime"):
            group = "project_runtime"
        elif file.is_relative_to(source):
            group = "mesa"
        elif file.is_relative_to(build):
            group = "generated"
        else:
            group = "external"
        counts[group] += 1
        sizes[group] += file.stat().st_size
    needed = re.findall(r"NEEDED\s+(\S+)", subprocess.run(
        ["objdump", "-p", str(binary)], check=True, text=True,
        stdout=subprocess.PIPE).stdout)
    if any("libdrm" in library for library in needed):
        raise RuntimeError("native compiler unexpectedly links libdrm")
    summary = {"mesa_version": "26.3.0", "target": target,
               "linked_objects": len(objects), "included_files": len(files),
               "file_counts": dict(sorted(counts.items())),
               "source_bytes": dict(sorted(sizes.items())),
               "runtime_libraries": needed,
               "binary_bytes": binary.stat().st_size,
               "note": "Haiku link and include closure; counts full source/header files. "
                       "Excludes build-only generators and is not a minimal port.",
               "runtime_override": override}
    (output / "objects.txt").write_text("\n".join(sorted(objects)) + "\n")
    (output / "files.txt").write_text("\n".join(str(file) for file in sorted(files)) + "\n")
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
