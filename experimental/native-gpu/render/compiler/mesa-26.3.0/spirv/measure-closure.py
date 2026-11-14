#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Measure the selected Mesa shader compiler link and include closure.

This records what the pinned host harness actually links. It is an upper
bound for a specialized N100 port, not a minimum source requirement.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess


def run(args, cwd):
    return subprocess.run(args, cwd=cwd, check=True, text=True,
                          stdout=subprocess.PIPE).stdout


def classify(path, mesa, build, project):
    if path.is_relative_to(mesa):
        return "mesa"
    if path.is_relative_to(build):
        return "generated"
    if path.is_relative_to(project):
        return "project"
    return "external"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("workspace", type=Path)
    parser.add_argument("output", type=Path, help="new, nonexistent directory")
    args = parser.parse_args()
    root = args.workspace.resolve()
    build = root / "build"
    mesa = root / "mesa-26.3.0"
    if (mesa / "VERSION").read_text().strip() != "26.3.0":
        parser.error("expected pinned Mesa 26.3.0 workspace")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)

    graph = (build / "build.ninja").read_text()
    target = "src/intel/compiler/brw/native-rcs-shaders"
    start = graph.index(f"build {target}: cpp_LINKER ")
    line = graph[start:].splitlines()[0]
    original_obj = line.split(" cpp_LINKER ", 1)[1].split(" | ", 1)[0]
    link = graph.index(" LINK_ARGS = ", start)
    link_args = shlex.split(graph[link:].splitlines()[0].split(" = ", 1)[1])
    # This harness uses the PCI-ID profile, not the fd/DRM discovery code in
    # intel_device_info.c. Section GC removes those uncalled functions.
    link_args = [arg for arg in link_args if "libdrm.so" not in arg
                 and not arg.startswith("-Wl,-rpath,")]
    commands = json.loads((build / "compile_commands.json").read_text())
    entry = next(item for item in commands if item["output"] == original_obj)
    command = shlex.split(entry["command"])
    command = command[:command.index("-MD")]
    source = Path(__file__).with_name("compile_spirv.c")
    identity = hashlib.sha256(source.read_bytes()).hexdigest()
    (output / "source_identity.h").write_text(
        f'#define NATIVE_RCS_SOURCE_SHA256 "{identity}"\n')
    main_obj = output / "compile_spirv.o"
    depfile = output / "compile_spirv.d"
    subprocess.run([command[0], "-I" + str(output), *command[1:],
                    "-MD", "-MF", str(depfile), "-o", str(main_obj),
                    "-c", str(source)], cwd=build, check=True)
    executable = output / "compiler"
    map_path = output / "link.map"
    subprocess.run(["c++", "-Wl,--gc-sections", f"-Wl,-Map={map_path}",
                    "-o", str(executable),
                    main_obj, *link_args], cwd=build, check=True)
    dynamic = run(["readelf", "-d", str(executable)], build)
    if "libdrm" in dynamic or " U drm" in run(["nm", "-D", str(executable)], build):
        raise RuntimeError("compiler still depends on libdrm")

    included = map_path.read_text().split("Discarded input sections", 1)[0]
    objects = set()
    for line in included.splitlines():
        match = re.match(r"^(\S+\.o)$", line)
        if match:
            objects.add(match.group(1))

    source_by_object = {entry["output"]: entry["file"] for entry in commands}
    missing = sorted(objects - source_by_object.keys())
    if missing:
        raise RuntimeError(f"no compile command for linked objects: {missing[:8]}")

    # Ninja stores compiler -MD results in .ninja_deps, removing the .d files.
    deps_text = run(["ninja", "-t", "deps", *sorted(objects)], build)
    deps = {}
    current = None
    for line in deps_text.splitlines():
        if line.startswith("    ") and current:
            deps[current].add((build / line.strip()).resolve())
        elif line.endswith(" (VALID)") or line.endswith(" (STALE)"):
            current = line.split(": #deps", 1)[0]
            deps[current] = set()
        else:
            current = None
    missing = sorted(objects - deps.keys())
    if missing:
        raise RuntimeError(f"no Ninja dependencies for linked objects: {missing[:8]}")

    sources = {(build / source_by_object[obj]).resolve() for obj in objects}
    # The SPIR-V harness is compiled outside Meson's graph, so parse its -MD
    # file separately. GCC escapes line breaks and spaces in this format.
    driver_deps = depfile.read_text().replace("\\\n", " ").split(":", 1)[1]
    harness_files = {source.resolve()}
    harness_files.update((build / path).resolve() for path in
                         shlex.split(driver_deps))
    files = sources | harness_files | set().union(*(deps[obj] for obj in objects))
    groups = Counter()
    sizes = Counter()
    project = Path(__file__).resolve().parents[6]
    for path in files:
        if not path.is_file():
            raise RuntimeError(f"included file missing: {path}")
        group = classify(path, mesa, build, project)
        groups[group] += 1
        sizes[group] += path.stat().st_size
    inventory = {
        "mesa_version": "26.3.0",
        "target": "Vulkan SPIR-V -> NIR -> N100 Intel code harness",
        "linked_objects": len(objects) + 1,
        "compiled_sources": len(sources) + 1,
        "files": len(files),
        "file_counts": dict(sorted(groups.items())),
        "source_bytes": dict(sorted(sizes.items())),
        "requires_libdrm": False,
        "note": "Host harness link/includes; excludes generators not in dep closure. "
                "Counts full files, including inactive code, and is not a minimal port. "
                "Section GC removes uncalled DRM discovery from the executable.",
    }
    (output / "summary.json").write_text(json.dumps(inventory, indent=2) + "\n")
    (output / "files.txt").write_text("\n".join(str(path) for path in sorted(files)) + "\n")
    (output / "objects.txt").write_text("\n".join(sorted(objects)) + "\n")
    print(json.dumps(inventory, indent=2))


if __name__ == "__main__":
    main()
