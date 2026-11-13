#!/usr/bin/env python3
"""Read-only build/reference graph and test inventory for staged relocation.

Usage: python3 tools/map-dependencies.py --output dependency-map.json
Scans textual repository files only. This is not a C++ semantic dependency graph.
"""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {".md", ".py", ".sh", ".mk", ".jam", ".yml", ".yaml", ".json",
                 ".txt", ".cpp", ".c", ".h", ".hpp", ".patch", ".xml"}
SOURCE_PREFIXES = ("patches/", "workstreams/", "tests/", "research/", "results/",
                   "installation/", "physical/", "diagnostics/", "tools/")
GROUPS = {
    "host_unit": ("tests/",),
    "physical": ("physical/", "tests/evidence/physical-"),
    "native_vm": ("installation/",),
    "research": ("research/",),
    "patches": ("patches/", "workstreams/gpu-next/"),
}
def classify(path):
    if path.startswith("tests/evidence/"):
        return "immutable_evidence"
    if path.startswith("tests/"):
        return "test_source"
    if path.startswith("workstreams/gpu-next/"):
        return "native_workstream"
    if path.startswith("patches/"):
        return "patch_series"
    for name, prefixes in GROUPS.items():
        if any(path.startswith(p) for p in prefixes):
            return name
    return "other"

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--output", default="-")
    args = p.parse_args()
    tracked = sorted(f for f in ROOT.rglob("*") if f.is_file()
                     and ".git" not in f.relative_to(ROOT).parts)
    rels = {f.relative_to(ROOT).as_posix(): f for f in tracked}
    refs = []
    # Detect explicit repository-relative paths only. Avoid guessing arbitrary C++ includes.
    pattern = re.compile(r"(?<![\w/])((?:" + "|".join(re.escape(p) for p in SOURCE_PREFIXES)
                         + r")[A-Za-z0-9_.+/@-]+)")
    for source, file in rels.items():
        if file.suffix.lower() not in TEXT_SUFFIXES and file.name not in ("Jamfile", "Makefile"):
            continue
        try:
            body = file.read_text(encoding="utf-8")
        except (UnicodeError, OSError):
            continue
        for target in sorted(set(pattern.findall(body))):
            refs.append({"from": source, "to": target,
                         "exists": target.rstrip("/").rstrip(".,:;") in rels
                         or (ROOT / target.rstrip("/").rstrip(".,:;")).exists()})
    by_group = {}
    for path in rels:
        group = classify(path)
        by_group[group] = by_group.get(group, 0) + 1
    report = {"files_seen": len(rels), "groups": by_group,
              "explicit_path_references": refs,
              "limitations": ["Heuristic path matches; may include comments or examples",
                              "Not a semantic include graph or applied-patch detector",
                              "Requires native build validation before any source relocation"]}
    data = json.dumps(report, indent=2) + "\n"
    if args.output == "-":
        print(data)
    else:
        Path(args.output).write_text(data, encoding="utf-8")
        print(f"Mapped {len(rels)} files and {len(refs)} explicit references")
if __name__ == "__main__":
    main()
