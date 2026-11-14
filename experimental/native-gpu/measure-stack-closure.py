#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Combine pinned compiler inputs with current Haiku owner/Vulkan depfiles."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import subprocess


def compiler_category(path):
    if path.startswith("/boot/home/test-work/mesa-26.3.0/"):
        return "mesa_source"
    if path.startswith("/boot/home/test-work/build-compiler/"):
        return "compiler_generated"
    if path.startswith("/boot/home/test-work/closure-current-022/include/spirv-tools/"):
        return "spirv_tools_header"
    if path.startswith("/boot/home/test-work/closure-current-022/src/"):
        return "compiler_project_runtime"
    if path.startswith("/boot/home/test-work/native-device-info-044/"):
        return "compiler_native_adaptation"
    return "compiler_sdk"


def metadata(host, paths):
    if any("\n" in p or "\t" in p for p in paths):
        raise ValueError("unsupported input path")
    marker = "RENKU_COMPILER_INPUT_PATHS"
    if marker in paths:
        raise ValueError("input path collides with marker")
    script = (
        'while IFS= read -r path; do\n'
        '  test -f "$path" || exit 21\n'
        '  bytes=$(wc -c < "$path") || exit 22\n'
        '  digest=$(sha256sum "$path") || exit 23\n'
        '  printf "%s\\t%s\\t%s\\n" "${digest%% *}" "$bytes" "$path"\n'
        f"done <<'{marker}'\n" + "\n".join(paths) + f"\n{marker}\n"
    )
    result = subprocess.run(["ssh", host, "bash", "-s"], input=script,
                            text=True, capture_output=True, timeout=180, check=True)
    found = {}
    for line in result.stdout.splitlines():
        digest, size, path = line.split("\t", 2)
        if len(digest) != 64 or path in found:
            raise ValueError("invalid compiler file metadata")
        found[path] = (digest, int(size))
    if set(found) != set(paths):
        raise ValueError("compiler file metadata incomplete")
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-files", type=Path, required=True)
    parser.add_argument("--owner-manifest", type=Path, required=True)
    parser.add_argument("--vulkan-manifest", type=Path, required=True)
    parser.add_argument("--host", default="renkuos-vm")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    paths = sorted(set(args.compiler_files.read_text().splitlines()))
    if len(paths) < 100 or len(paths) != len(args.compiler_files.read_text().splitlines()):
        raise ValueError("compiler input list incomplete or duplicated")
    found = metadata(args.host, paths)
    files = {path: {"path": path, "category": compiler_category(path),
                    "bytes": found[path][1], "sha256": found[path][0]}
             for path in paths}
    for manifest_path in (args.owner_manifest, args.vulkan_manifest):
        manifest = json.loads(manifest_path.read_text())
        if manifest["host"] != args.host:
            raise ValueError("manifests must describe the same host")
        for entry in manifest["files"]:
            previous = files.get(entry["path"])
            if previous and (previous["sha256"], previous["bytes"]) != (
                    entry["sha256"], entry["bytes"]):
                raise ValueError("inconsistent shared input: " + entry["path"])
            if not previous:
                files[entry["path"]] = entry
    groups = defaultdict(lambda: {"files": 0, "bytes": 0})
    for entry in files.values():
        group = groups[entry["category"]]
        group["files"] += 1
        group["bytes"] += entry["bytes"]
    generated = {"compiler_generated", "generated_owner"}
    probes = {"probe_source"}
    sdk = {"compiler_sdk", "haiku_sdk"}
    runtime = [e for e in files.values()
               if e["category"] not in generated | probes | sdk]
    def unique_bytes(entries):
        unique = {}
        for entry in entries:
            digest, size = entry["sha256"], entry["bytes"]
            if digest in unique and unique[digest] != size:
                raise ValueError("identical hash with different sizes")
            unique[digest] = size
        return sum(unique.values())
    result = {
        "schema": 1,
        "scope": "compiler link/include files plus current owner and Vulkan direct compile inputs",
        "host": args.host,
        "compiler_files": str(args.compiler_files),
        "compiler_file_list_sha256": hashlib.sha256(
            args.compiler_files.read_bytes()).hexdigest(),
        "input_manifests": [str(args.owner_manifest), str(args.vulkan_manifest)],
        "totals_by_category": dict(sorted(groups.items())),
        "path_union_files": len(files),
        "path_union_bytes": sum(e["bytes"] for e in files.values()),
        "unique_content_bytes": unique_bytes(list(files.values())),
        "runtime_canonical_unique_bytes_excluding_sdk_generated_and_probes": unique_bytes(runtime),
        "model_tokenizer": None,
        "model_tokens": None,
        "unmeasured": ["SPIRV-Tools implementation source",
                       "build-only generators and their source",
                       "all RenkuOS kernel sources outside the owner closure",
                       "development-model tokenizer tokens"],
        "files": [files[path] for path in sorted(files)],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in (
        "path_union_files", "path_union_bytes", "unique_content_bytes",
        "runtime_canonical_unique_bytes_excluding_sdk_generated_and_probes")},
        sort_keys=True))


if __name__ == "__main__":
    main()
