#!/usr/bin/env python3
"""Verify the published file set and digests, not the truth of submitted claims."""

import hashlib
import json
from pathlib import Path


def main():
    root = Path(__file__).resolve().parent
    expected = {}
    for line in (root / "published.sha256").read_text().splitlines():
        digest, name = line.split("  ", 1)
        relative = Path(name)
        if relative.is_absolute() or ".." in relative.parts or name in expected:
            raise SystemExit(f"Invalid manifest entry: {name}")
        expected[name] = digest
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*")
              if p.is_file() and p.name != "published.sha256"}
    if actual != set(expected):
        raise SystemExit(f"File set differs: {sorted(actual ^ set(expected))}")
    for name, digest in expected.items():
        path = root / name
        if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise SystemExit(f"Digest mismatch or symbolic link: {name}")
        if name.endswith(".json"):
            json.loads(path.read_text())
    probes = root / "evidence/probes"
    requests = {p.name.removesuffix(".request.json")
                for p in probes.glob("*.request.json")}
    responses = {p.name.removesuffix(".response.json")
                 for p in probes.glob("*.response.json")}
    if len(requests) != 20 or requests != responses:
        raise SystemExit("Expected exactly 20 paired probes")
    print(f"Verified {len(expected)} files and 20 paired probes; claims are not certified.")


if __name__ == "__main__":
    main()
