#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Audit patch targets and inline local Markdown destinations.

Usage: python3 tools/audit-repository.py --output audit.json [--strict]
Strict mode rejects unexpected missing links and patch inventory errors.
One byte-pinned broken link in archived failed-model evidence is reported
separately, never repaired by altering the historical submission.
This does not validate heading anchors, external URLs or reference-style links.
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r'(?<!!)\[[^\]]*\]\((?:<([^>]+)>|([^\s)]+))(?:\s+[^)]*)?\)')
TARGET = re.compile(r'^\+\+\+ b/(\S+)', re.M)
EXPECTED_PATCH_COUNT = 118
# Exact source path + destination + original Git blob, not a directory exclusion.
# Verified in the unchanged PR base 30dfdb32957215b4c59da3ac763ba14ab0c482d0.
ARCHIVED_MISSING_LINKS = {
    (
        'research/local-model-evaluation/qwen3-coder-2026-11-07/evidence/'
        'game-review-artifacts/final-review-014/submitted/README.submitted.md',
        'IMPLEMENTATION_DETAILS.md',
        'e87501cbb5294652ed438f28868d96a71690356a',
    ): 'Missing attachment in preserved failed-model submission; keep original evidence bytes.',
}


def git_blob_sha(raw):
    """Match this SHA-1 Git repository's content identifier; not a security signature."""
    return hashlib.sha1(b'blob ' + str(len(raw)).encode('ascii') + b'\0' + raw).hexdigest()


def inspect(root=None):
    root = ROOT if root is None else Path(root)
    missing, archived, unexpected = [], [], []
    checked = 0
    for doc in sorted(root.rglob('*.md')):
        if '.git' in doc.relative_to(root).parts:
            continue
        raw = doc.read_bytes()
        body = raw.decode('utf-8', errors='replace')
        body = re.sub(r'^(`{3,}|~{3,}).*?^\1[^\n]*$', '', body, flags=re.M | re.S)
        for match in LINK.finditer(body):
            dest = (match.group(1) or match.group(2)).strip()
            if dest.startswith(('#', 'mailto:', 'data:', '//')):
                continue
            parts = urlsplit(dest)
            if parts.scheme or parts.netloc:
                continue
            path = unquote(parts.path)
            if not path:
                continue
            checked += 1
            if not (doc.parent / path).exists():
                source = doc.relative_to(root).as_posix()
                blob = git_blob_sha(raw)
                record = {'source': source, 'destination': dest, 'source_git_blob': blob}
                missing.append(record)
                reason = ARCHIVED_MISSING_LINKS.get((source, dest, blob))
                if reason is not None:
                    archived.append({**record, 'reason': reason})
                else:
                    unexpected.append(record)
    patches = []
    for patch in sorted((root / 'patches').glob('*.patch')):
        body = patch.read_text(encoding='utf-8', errors='replace')
        targets = sorted(set(TARGET.findall(body)))
        patches.append({'path': patch.relative_to(root).as_posix(),
                        'targets': targets, 'missing_targets': not bool(targets)})
    numbers = [int(Path(p['path']).name[:4]) for p in patches]
    return {
        'markdown_relative_links_checked': checked,
        'missing_relative_links': missing,
        'archived_missing_relative_links': archived,
        'unexpected_missing_relative_links': unexpected,
        'patch_count': len(patches),
        'expected_patch_count': EXPECTED_PATCH_COUNT,
        'missing_patch_numbers': sorted(set(range(1, max(numbers, default=0) + 1)) - set(numbers)),
        'patches': patches,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', help='Optional JSON output path')
    parser.add_argument('--strict', action='store_true', help='Fail on unexpected findings')
    args = parser.parse_args()
    report = inspect()
    encoded = json.dumps(report, indent=2) + '\n'
    if args.output:
        Path(args.output).write_text(encoded, encoding='utf-8')
    else:
        print(encoded)
    issues = len(report['unexpected_missing_relative_links'])
    archived = len(report['archived_missing_relative_links'])
    malformed = sum(p['missing_targets'] for p in report['patches'])
    print(f"Audit: {report['patch_count']} patches, {issues} unexpected missing links, "
          f"{archived} byte-pinned archived missing links, {malformed} patches without targets",
          file=sys.stderr)
    for record in report['missing_relative_links']:
        print(f"MISSING: {record['source']} -> {record['destination']}", file=sys.stderr)
    if args.strict and (issues or malformed or report['patch_count'] != EXPECTED_PATCH_COUNT):
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
