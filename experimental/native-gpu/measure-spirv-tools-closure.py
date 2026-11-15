#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Inventory the pinned SPIRV-Tools shared target and merge its source hashes."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import shlex


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def unique_bytes(entries):
    hashes = {}
    for entry in entries:
        key = entry["sha256"]
        if key in hashes and hashes[key] != entry["bytes"]:
            raise ValueError("hash collision with different lengths")
        hashes[key] = entry["bytes"]
    return sum(hashes.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--tools-source", type=Path, required=True)
    parser.add_argument("--headers-source", type=Path, required=True)
    parser.add_argument("--stack-manifest", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build = args.build.resolve()
    tools = args.tools_source.resolve()
    headers = args.headers_source.resolve()
    commands = json.loads((build / "compile_commands.json").read_text())
    selected = [entry for entry in commands
                if "/SPIRV-Tools-shared.dir/" in entry["command"]]
    if len(selected) < 70:
        raise ValueError("SPIRV-Tools shared target is incomplete")
    files = set()
    for entry in selected:
        source = Path(entry["file"]).resolve()
        object_file = (build / entry["output"]).resolve()
        depfile = Path(str(object_file) + ".d")
        if not source.is_file() or not depfile.is_file():
            raise ValueError("missing compiled source or dependency file")
        deps = depfile.read_text().replace("\\\n", " ").split(":", 1)[1]
        inputs = {Path(path).resolve() for path in shlex.split(deps)}
        if source not in inputs:
            raise ValueError("compiled source absent from dependency file")
        files.update(inputs)
    entries = []
    for path in sorted(files):
        if not path.is_file():
            raise ValueError("missing input: " + str(path))
        category = ("spirv_tools_source" if path.is_relative_to(tools) else
                    "spirv_headers_source" if path.is_relative_to(headers) else
                    "spirv_tools_generated" if path.is_relative_to(build) else
                    "host_sdk")
        entries.append({"path": str(path), "category": category,
                        "bytes": path.stat().st_size, "sha256": digest(path)})
    source_entries = [entry for entry in entries if entry["category"] in
                      ("spirv_tools_source", "spirv_headers_source")]
    stack = json.loads(args.stack_manifest.read_text())
    excluded = {"compiler_generated", "generated_owner", "probe_source",
                "compiler_sdk", "haiku_sdk"}
    prior = [entry for entry in stack["files"]
             if entry["category"] not in excluded]
    previous_bytes = unique_bytes(prior)
    if previous_bytes != stack[
            "runtime_canonical_unique_bytes_excluding_sdk_generated_and_probes"]:
        raise ValueError("prior stack manifest total does not reproduce")
    combined_bytes = unique_bytes(prior + source_entries)
    categories = defaultdict(lambda: {"files": 0, "bytes": 0})
    for entry in entries:
        category = categories[entry["category"]]
        category["files"] += 1
        category["bytes"] += entry["bytes"]
    result = {
        "schema": 1,
        "scope": "host reproduction of the pinned SPIRV-Tools shared target",
        "tools_source": str(tools), "headers_source": str(headers),
        "host_build": str(build), "prior_stack_manifest": str(args.stack_manifest),
        "compiled_sources": len(selected),
        "dependency_files": len(entries),
        "totals_by_category": dict(sorted(categories.items())),
        "project_source_files": len(source_entries),
        "project_source_bytes_by_path": sum(e["bytes"] for e in source_entries),
        "project_source_unique_bytes": unique_bytes(source_entries),
        "prior_stack_unique_bytes": previous_bytes,
        "combined_selected_runtime_unique_bytes": combined_bytes,
        "added_unique_bytes": combined_bytes - previous_bytes,
        "limitation": "Host build source selection and headers; Haiku package build dependency files unavailable",
        "files": entries,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({key: result[key] for key in (
        "compiled_sources", "project_source_files",
        "project_source_unique_bytes", "prior_stack_unique_bytes",
        "combined_selected_runtime_unique_bytes", "added_unique_bytes")},
        sort_keys=True))


if __name__ == "__main__":
    main()
