#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent
manifest = json.loads((root / 'upstream-manifest.json').read_text())
paths = [entry['path'] for entry in manifest['files']]
if len(paths) != len(set(paths)):
    raise SystemExit('Duplicate path in upstream manifest')
for entry in manifest['files']:
    data = (root / 'upstream' / entry['path']).read_bytes()
    blob = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
    if blob != entry['gitBlob'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise SystemExit('Upstream hash mismatch: ' + entry['path'])
print('Verified %d unchanged Linux v6.6 files' % len(manifest['files']))
