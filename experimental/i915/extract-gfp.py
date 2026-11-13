#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract the complete original predicate without Linux VM/zone services."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parent
entry = next(f for f in json.loads((root/'upstream-manifest.json').read_text())['files']
             if f['path'] == 'linux/gfp.h')
data = (root/'upstream/linux/gfp.h').read_bytes()
assert hashlib.sha256(data).hexdigest() == entry['sha256']
source = data.decode()
start = source.index('static inline bool gfpflags_allow_blocking(')
end = source.index('\n}', start) + 2
function = source[start:end]
assert function.count('{') == function.count('}') == 1
(root/'build').mkdir(exist_ok=True)
(root/'build/gfp_helpers.h').write_text('/* SPDX-License-Identifier: GPL-2.0 */\n'
    '/* Complete original Linux v6.6 blocking predicate; no VM policy. */\n'
    + function + '\n')
(root/'build/gfp_helpers.json').write_text(json.dumps({'source': entry,
    'function': 'gfpflags_allow_blocking',
    'originalFunctionSHA256': hashlib.sha256(function.encode()).hexdigest(),
    'completeGfpPort': False}, indent=2) + '\n')
print('Extracted complete original GFP blocking predicate')
