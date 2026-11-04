#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract unchanged production method bodies for an isolated callback fixture.

This is an algorithm test, not a replacement for compiling/loading the manager.
Fail closed if the expected source layout changes. Retain original attribution.
"""
import argparse
import hashlib
import pathlib
import re


def extract(source, output):
    data = source.read_text()
    notice = re.match(r'/\*.*?\*/', data, re.S)
    if notice is None:
        raise SystemExit('missing source license notice')
    parts = [notice.group(), '// Production source SHA-256: '
             + hashlib.sha256(source.read_bytes()).hexdigest()]
    flag = re.findall(r'^#define BIND_APERTURE\s+[^\n]+$', data, re.M)
    if len(flag) != 1:
        raise SystemExit('expected exactly one BIND_APERTURE definition')
    parts.extend(flag)
    for name, result in [('_AdaptToReserved', 'bool'), ('UnbindMemory', 'status_t')]:
        matches = re.findall(r'^' + result + r'\nAperture::' + name
                             + r'\([^)]*\)\n\{.*?^}\n', data, re.M | re.S)
        if len(matches) != 1:
            raise SystemExit(f'expected exactly one complete method: {name}')
        parts.append(matches[0])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n\n'.join(parts))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    args = parser.parse_args()
    extract(args.source, args.output)
