#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Fetch the hash-pinned MIT reference files needed by the context-image oracle."""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen

PIN = '028ef9c96e96197026887c0f092424679298aae8'
FILES = {
    'intel_lrc.c': '1c6d88c0cf02125e02d4826b09d9bbd9c1acebaab6d53e74ac36462a24eac3ba',
    'intel_lrc.h': 'e71534746b6f6720c978c0096316c51873f5afc2974d94cd4979303384a6f5b3',
    'intel_lrc_reg.h': '671b2adfb2825597425eea515b892cba1365f658d552f9a90439182f0f7f21c0',
    'intel_engine_regs.h': 'e6ae20bc2946a252a948628df6a84522c19dd45d537f5efa0e5c55d08e82961a',
    'intel_gpu_commands.h': 'd8d14b3edefb858418eb52506f5f7598ee2cd6bd8affa659e43faaf11c85e77c',
    'intel_gt_regs.h': '2d131eb5839396c099e8c63b52e8f998407936d8a34df4a547080fc13f5a91b9',
    'gen8_engine_cs.c': 'd605b5e4b74d76e29be1db9fb22b2f16d52f8ed96ad2c5746f850665f10f2c72',
}
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True, help='fresh output directory')
parser.add_argument('--offline-from', type=Path, help='verify existing files instead of downloading')
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=False)
records = {}
for name, digest in FILES.items():
    url = f'https://raw.githubusercontent.com/torvalds/linux/{PIN}/drivers/gpu/drm/i915/gt/{name}'
    if args.offline_from:
        data = (args.offline_from / name).read_bytes()
    else:
        with urlopen(url, timeout=30) as response:
            data = response.read(2 * 1024 * 1024 + 1)
    if len(data) > 2 * 1024 * 1024 or hashlib.sha256(data).hexdigest() != digest:
        raise SystemExit(f'reference size/hash mismatch: {name}')
    (args.output / name).write_bytes(data)
    records['gt/' + name] = {'url': url, 'sha256': digest}
    print(f'PASS {name} {digest}')
(args.output / 'references.json').write_text(json.dumps(records, indent=2) + '\n')
