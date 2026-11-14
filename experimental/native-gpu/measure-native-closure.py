#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Measure exact native owner/bridge compile inputs from target depfiles."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import shlex
import subprocess

def dependencies(path, bundle):
    contents = path.read_text().replace("\\\n", " ")
    if ":" not in contents:
        raise ValueError(f"not a GCC depfile: {path}")
    for item in shlex.split(contents.split(":", 1)[1]):
        yield posixpath.normpath(item if item.startswith("/") else f"{bundle}/{item}")

def category(path, bundle, owner_source):
    if path == f"{bundle}/IntelMemoryOwner-execution.cpp":
        return "generated_owner"
    if path.startswith(bundle + "/"):
        return "native_bundle"
    if path.startswith(owner_source + "/"):
        return "haiku_owner_source"
    if path.startswith("/boot/system/develop/"):
        return "haiku_sdk"
    return "other_external"

def remote_metadata(host, paths):
    marker = "RENKU_NATIVE_CLOSURE_PATHS"
    if marker in paths:
        raise ValueError("heredoc marker collided with a path")
    script = (
        'while IFS= read -r path; do\n'
        '  test -f "$path" || exit 21\n'
        '  bytes=$(wc -c < "$path") || exit 22\n'
        '  digest=$(sha256sum "$path") || exit 23\n'
        '  printf "%s\\t%s\\t%s\\n" "${digest%% *}" "$bytes" "$path"\n'
        f'done <<\'{marker}\'\n' + "\n".join(paths) + f"\n{marker}\n"
    )
    result = subprocess.run(["ssh", host, "bash", "-s"], input=script,
                            text=True, capture_output=True, timeout=180, check=True)
    records = {}
    for line in result.stdout.splitlines():
        digest, size, path = line.split("\t", 2)
        if len(digest) != 64 or path in records:
            raise ValueError("invalid or duplicate remote metadata")
        records[path] = (digest, int(size))
    if set(records) != set(paths):
        raise ValueError("remote metadata is incomplete")
    return records

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--deps", type=Path, required=True)
    parser.add_argument("--bundle", required=True)
    parser.add_argument("--host", default="renkuos-n100")
    parser.add_argument("--owner-source", default="/boot/home/i915-mapping-002/source")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    bundle = str(PurePosixPath(args.bundle))
    depfiles = sorted(args.deps.glob("*.d"))
    if {p.name for p in depfiles} != {"owner.d", "bridge.d", "native_page_bridge.d", "kernel_cpp.d"}:
        raise SystemExit("expected four owner/bridge runtime depfiles")
    paths = sorted({item for depfile in depfiles for item in dependencies(depfile, bundle)})
    owner_source = str(PurePosixPath(args.owner_source))
    extras = [f"{bundle}/LICENSE.intel",
              f"{owner_source}/src/add-ons/kernel/bus_managers/intel_memory_owner/IntelMemoryOwner.cpp"]
    paths = sorted(set(paths + extras))
    raw = remote_metadata(args.host, paths)
    files = [{"path": path, "category": category(path, bundle, owner_source),
              "bytes": raw[path][1], "sha256": raw[path][0]}
             for path in paths]
    totals = {}
    for entry in files:
        group = totals.setdefault(entry["category"], {"files": 0, "bytes": 0})
        group["files"] += 1; group["bytes"] += entry["bytes"]
    manifest = {"schema": 1, "scope": "native owner/bridge runtime compile inputs and two provenance extras",
                "host": args.host, "bundle": bundle, "owner_source": owner_source,
                "depfile_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in depfiles},
                "totals": totals, "files": files,
                "model_tokenizer": None, "model_tokens": None,
                "token_note": "The development model tokenizer is not available in this environment."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(totals, sort_keys=True))

if __name__ == "__main__":
    main()
