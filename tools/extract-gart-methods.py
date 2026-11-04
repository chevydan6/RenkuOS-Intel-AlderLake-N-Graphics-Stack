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


def extract(source, output, lifecycle=False, teardown=False, allocation=False):
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
    names = ['_AdaptToReserved', 'UnbindMemory']
    if allocation:
        names = ['_AdaptToReserved', 'AllocateMemory', '_Free']
    if lifecycle:
        names += ['BindMemory', 'DeleteMemory', '_Remove']
    if lifecycle or allocation:
        flags = re.findall(r'^#define ALLOCATED_APERTURE\s+[^\n]+$', data, re.M)
        if len(flags) != 1:
            raise SystemExit('expected exactly one ALLOCATED_APERTURE definition')
        parts.extend(flags)
    if teardown:
        names += ['DeleteAllMemory']
    for name in names:
        matches = re.findall(r'^(?:bool|void|status_t)\nAperture::' + name
                             + r'\([^)]*\)\n\{.*?^}\n', data, re.M | re.S)
        if len(matches) != 1:
            raise SystemExit(f'expected exactly one complete method: {name}')
        parts.append(matches[0])
    if lifecycle:
        matches = re.findall(r'^static status_t\nbind_aperture\([^)]*\)\n\{.*?^}\n',
                             data, re.M | re.S)
        if len(matches) != 1:
            raise SystemExit('expected exactly one bind_aperture function')
        parts.append(matches[0])
    if teardown:
        matches = re.findall(r'^static status_t\nunmap_aperture\([^)]*\)\n\{.*?^}\n',
                             data, re.M | re.S)
        if len(matches) != 1:
            raise SystemExit('expected exactly one unmap_aperture function')
        parts.append(matches[0])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n\n'.join(parts))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    parser.add_argument('--lifecycle', action='store_true')
    parser.add_argument('--teardown', action='store_true')
    parser.add_argument('--allocation', action='store_true')
    args = parser.parse_args()
    if args.allocation and (args.lifecycle or args.teardown):
        parser.error('--allocation is a separate fixture')
    extract(args.source, args.output, args.lifecycle, args.teardown, args.allocation)
