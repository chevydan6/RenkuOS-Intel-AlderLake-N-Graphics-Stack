#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Hash native Vulkan library and probe direct compile inputs on RenkuOS."""
import argparse
import hashlib
import json
from pathlib import Path
import posixpath
import shlex
import subprocess

def paths_from_depfile(path):
    contents = path.read_text().replace("\\\n", " ")
    if ":" not in contents:
        raise ValueError(f"invalid GCC depfile: {path}")
    return (posixpath.normpath(p) for p in shlex.split(contents.split(":", 1)[1]))

def category(path, bundle):
    if path.startswith(bundle + "/include/"):
        return "khronos_headers"
    if path == bundle + "/vulkan/NativeVulkan.cpp":
        return "driver_source"
    if path.startswith(bundle + "/render/draw/"):
        return "intel_draw_fixture"
    if path.startswith(bundle + "/render/"):
        return "native_render_headers"
    if path in (bundle + "/RenderReport.h", bundle + "/ResourceCommand.h"):
        return "native_abi"
    if path.startswith(bundle + "/vulkan/"):
        return "probe_source"
    return "other"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--deps", type=Path, required=True)
    parser.add_argument("--bundle", default="/boot/home/native-vulkan-abi-001")
    parser.add_argument("--host", default="renkuos-n100")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    depfiles = sorted(args.deps.glob("*.d"))
    names = {p.name for p in depfiles}
    required = {"libvulkan.d"}
    api_probes = {"api_probe.d", "compiler_pipeline_api_probe.d"}
    optional = {"triangle_probe.d", "fence_probe.d", "command_reset_probe.d",
                "compiler_pipeline_probe.d"}
    if (not required.issubset(names) or not (names & api_probes)
            or not names.issubset(required | api_probes | optional)):
        raise ValueError("expected library and probe GCC depfiles")
    paths = sorted({p for dep in depfiles for p in paths_from_depfile(dep)})
    if args.bundle + "/render/draw/RcsDrawGenerated.h" in paths:
        paths.append(args.bundle + "/render/draw/LICENSE-MIT-Intel.txt")
        paths.sort()
    if any("\n" in p or "\t" in p for p in paths):
        raise ValueError("unsupported path in depfile")
    marker = "RENKU_VULKAN_SOURCE_PATHS"
    script = ('while IFS= read -r path; do\n'
              ' test -f "$path" || exit 21\n'
              ' bytes=$(wc -c < "$path") || exit 22\n'
              ' digest=$(sha256sum "$path") || exit 23\n'
              ' printf "%s\\t%s\\t%s\\n" "${digest%% *}" "$bytes" "$path"\n'
              f'done <<\'{marker}\'\n' + "\n".join(paths) + f"\n{marker}\n")
    result = subprocess.run(["ssh", args.host, "bash", "-s"], input=script,
                            text=True, capture_output=True, timeout=120, check=True)
    records = {}
    for line in result.stdout.splitlines():
        digest, size, path = line.split("\t", 2)
        if len(digest) != 64 or path in records:
            raise ValueError("invalid or duplicate physical metadata")
        records[path] = (digest, int(size))
    if set(records) != set(paths):
        raise ValueError("incomplete physical metadata")
    files = [{"path": p, "category": category(p, args.bundle),
              "bytes": records[p][1], "sha256": records[p][0]} for p in paths]
    totals = {}
    for entry in files:
        group = totals.setdefault(entry["category"], {"files": 0, "bytes": 0})
        group["files"] += 1
        group["bytes"] += entry["bytes"]
    manifest = {"schema": 1, "scope": "native Vulkan library and probe direct compile inputs",
                "host": args.host, "bundle": args.bundle,
                "depfile_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                   for p in depfiles},
                "totals": totals, "files": files,
                "model_tokenizer": None, "model_tokens": None}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(totals, sort_keys=True))

if __name__ == "__main__":
    main()
