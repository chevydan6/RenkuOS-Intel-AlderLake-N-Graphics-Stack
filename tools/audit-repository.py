#!/usr/bin/env python3
"""Read-only repository inventory. No files are changed.

Usage: python3 tools/audit-repository.py --output audit.json [--strict]
Default mode reports findings without failing on legacy links. Strict mode
fails on missing local Markdown links, malformed patch targets, or patch count
drift. Run on a full Git checkout, not a sparse checkout.
"""
import argparse
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r'(?<!!)\[[^\]]*\]\((?:<([^>]+)>|([^\s)]+))(?:\s+[^)]*)?\)')
TARGET = re.compile(r'^\+\+\+ b/(\S+)', re.M)
EXPECTED_PATCH_COUNT = 118

def inspect():
    missing = []
    checked = 0
    for doc in sorted(ROOT.rglob("*.md")):
        if ".git" in doc.relative_to(ROOT).parts:
            continue
        body = doc.read_text(encoding="utf-8", errors="replace")
        # Ignore fenced code blocks; links inside examples are not live references.
        body = re.sub(r"^(`{3,}|~{3,}).*?^\1[^\n]*$", "", body,
                      flags=re.M | re.S)
        for match in LINK.finditer(body):
            dest = (match.group(1) or match.group(2)).strip()
            if dest.startswith(("#", "mailto:", "data:", "//")):
                continue
            parts = urlsplit(dest)
            if parts.scheme or parts.netloc:
                continue
            path = unquote(parts.path)
            if not path:
                continue
            checked += 1
            candidate = doc.parent / path
            if not candidate.exists():
                missing.append({"source": str(doc.relative_to(ROOT)),
                                "destination": dest})
    patches = []
    for patch in sorted((ROOT / "patches").glob("*.patch")):
        body = patch.read_text(encoding="utf-8", errors="replace")
        targets = sorted(set(TARGET.findall(body)))
        patches.append({"path": str(patch.relative_to(ROOT)),
                        "targets": targets, "missing_targets": not bool(targets)})
    numbers = [int(Path(p["path"]).name[:4]) for p in patches]
    return {"markdown_relative_links_checked": checked,
            "missing_relative_links": missing,
            "patch_count": len(patches),
            "expected_patch_count": EXPECTED_PATCH_COUNT,
            "missing_patch_numbers": sorted(set(range(1, max(numbers, default=0) + 1)) - set(numbers)),
            "patches": patches}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", help="Optional JSON output path")
    parser.add_argument("--strict", action="store_true", help="Fail on findings")
    args = parser.parse_args()
    report = inspect()
    encoded = json.dumps(report, indent=2) + "\n"
    if args.output:
        Path(args.output).write_text(encoded, encoding="utf-8")
    else:
        print(encoded)
    issues = len(report["missing_relative_links"])
    malformed = sum(p["missing_targets"] for p in report["patches"])
    print(f"Audit: {report['patch_count']} patches, {issues} missing links, "
          f"{malformed} patches without targets", file=sys.stderr)
    if args.strict and (issues or malformed or report["patch_count"] != EXPECTED_PATCH_COUNT):
        return 1
    return 0

if __name__ == "__main__":
    sys.exit(main())
