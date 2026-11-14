#!/usr/bin/env python3
"""Measure direct compiler inputs for a built Ninja target, excluding link closure."""

import argparse
import hashlib
import json
import pathlib
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=pathlib.Path)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("target")
    parser.add_argument("--ninja", default="ninja")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    build, source = args.build.resolve(), args.source.resolve()
    def ninja(*extra):
        return subprocess.check_output([args.ninja, "-C", str(build), "-t", *extra], text=True)

    commands = ninja("commands", args.target)
    objects = set()
    for line in commands.splitlines():
        if " -c " not in line:
            continue
        match = re.search(r"(?:^| )-o ([^ ]+\.o)(?: |$)", line)
        if match:
            objects.add(match.group(1))

    deps = ninja("deps")
    current = None
    selected = {}
    for line in deps.splitlines():
        if not line.startswith(" "):
            current = line.split(": #deps ", 1)[0] if ": #deps " in line else None
            if current in objects:
                selected[current] = []
        elif current in objects:
            selected[current].append(line.strip())
    if selected.keys() != objects:
        raise SystemExit(f"missing depfiles for {len(objects - selected.keys())} objects")

    files = {}
    for dependencies in selected.values():
        for name in dependencies:
            path = (build / name).resolve()
            if path.is_relative_to(source):
                category, relative = "mesa_source", str(path.relative_to(source))
            elif path.is_relative_to(build):
                category, relative = "generated_build_input", str(path.relative_to(build))
            else:
                category, relative = "external_header", str(path)
            if not path.is_file():
                raise SystemExit(f"missing dependency: {path}")
            files[(category, relative)] = path
    rows = [{"category": category, "path": relative,
             "bytes": path.stat().st_size,
             "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
            for (category, relative), path in sorted(files.items())]
    totals = {category: {"files": sum(row["category"] == category for row in rows),
                         "bytes": sum(row["bytes"] for row in rows if row["category"] == category)}
              for category in ("mesa_source", "generated_build_input", "external_header")}
    args.output.write_text(json.dumps({"target": args.target, "compiled_objects": len(objects),
                                       "totals": totals, "files": rows}, indent=2) + "\n")
    print(json.dumps({"compiled_objects": len(objects), "totals": totals}))


if __name__ == "__main__":
    main()
