#!/usr/bin/env python3
"""Read-only repository link and patch target audit. Run from repository root."""
from pathlib import Path
import re
import json
import sys
from urllib.parse import unquote

root = Path(__file__).resolve().parents[1]
missing = []
links = 0
for doc in root.rglob("*.md"):
    if ".git" in doc.parts:
        continue
    text = doc.read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"(?<!!)\[[^\]]*\]\(([^)]+)\)", text):
        dest = m.group(1).strip().split()[0].strip("<>")
        if not dest or dest.startswith(("https:", "http:", "mailto:", "#", "data:")):
            continue
        links += 1
        local = unquote(dest.split("#", 1)[0].split("?", 1)[0])
        if not local:
            continue
        if not (doc.parent / local).exists():
            missing.append({"source": str(doc.relative_to(root)), "destination": dest})
patches = []
for patch in sorted((root / "patches").glob("*.patch")):
    text = patch.read_text(encoding="utf-8", errors="replace")
    targets = sorted(set(re.findall(r"^\+\+\+ b/(\S+)", text, re.M)))
    patches.append({"patch": str(patch.relative_to(root)), "targets": targets,
                    "missing_targets": not bool(targets)})
report = {"markdown_relative_links_checked": links, "missing_relative_links": missing,
          "patches_inspected": len(patches), "patches": patches}
print(json.dumps(report, indent=2))
sys.exit(1 if missing or any(p["missing_targets"] for p in patches) else 0)
