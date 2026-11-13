#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract an original, complete pure helper; never synthesize bitmap services."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parent
entry = next(f for f in json.loads((root/'upstream-manifest.json').read_text())['files']
             if f['path'] == 'lib/bitmap.c')
data = (root/'upstream/lib/bitmap.c').read_bytes()
assert hashlib.sha256(data).hexdigest() == entry['sha256']
source = data.decode()
start = source.index('bool __bitmap_equal(')
end = source.index('\nEXPORT_SYMBOL(__bitmap_equal);', start)
function = source[start:end]
assert function.endswith('}') and function.count('{') == function.count('}')
(root/'build').mkdir(exist_ok=True)
(root/'build/bitmap_equal_slice.c').write_text('// SPDX-License-Identifier: GPL-2.0-only\n'
    '/* Complete original Linux v6.6 __bitmap_equal; lib/bitmap.c contains other\n'
    ' * OS-dependent services not supplied by this extraction. */\n'
    '#include <linux/bitmap.h>\n'+function+'\n')
(root/'build/bitmap_equal_slice.json').write_text(json.dumps({
    'source': entry, 'function': '__bitmap_equal',
    'originalFunctionSHA256': hashlib.sha256(function.encode()).hexdigest(),
    'completeLibBitmapPort': False}, indent=2)+'\n')
print('Extracted complete original __bitmap_equal')
