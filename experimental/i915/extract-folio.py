#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract complete original pure folio helpers over native shape queries."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parent
manifest = json.loads((root/'upstream-manifest.json').read_text())
records = []; fragments = []
for path, signatures in [
    ('linux/mm.h', ['static inline size_t folio_size(', 'static inline unsigned int folio_shift(',
                   'static inline unsigned long folio_pfn(', 'static inline struct folio *virt_to_folio(',
                   'static inline unsigned long page_size(', 'static inline unsigned int page_shift(']),
    ('linux/page-flags.h', ['#define folio_page(folio, n)']),
]:
    entry = next(f for f in manifest['files'] if f['path'] == path)
    data = (root/'upstream'/path).read_bytes()
    assert hashlib.sha256(data).hexdigest() == entry['sha256']
    source = data.decode()
    for signature in signatures:
        start = source.index(signature)
        end = source.index('\n', start) if signature.startswith('#define') else source.index('\n}', start) + 2
        fragment = source[start:end]; fragments.append(fragment)
        records.append({'source': entry, 'signature': signature, 'sha256': hashlib.sha256(fragment.encode()).hexdigest()})
# Preserve original const-selection macro, substituting only its backend primitive.
source = (root/'upstream/linux/page-flags.h').read_text()
start = source.index('#define page_folio(p)'); end = source.index('\n\n', start)
fragments.insert(0, source[start:end])
records.append({'source': 'linux/page-flags.h', 'signature': '#define page_folio(p)',
                'sha256': hashlib.sha256(source[start:end].encode()).hexdigest()})
(root/'build').mkdir(exist_ok=True)
(root/'build/folio_helpers.h').write_text('/* SPDX-License-Identifier: GPL-2.0 */\n'
    '/* Complete original v6.6 helper bodies; native registry supplies shape. */\n'
    '#define _compound_head(p) ((unsigned long)renku_linux_page_folio(p))\n'+'\n\n'.join(fragments)+'\n')
(root/'build/folio_helpers.json').write_text(json.dumps({'helpers': records,
    'completeLinuxMMPort': False}, indent=2)+'\n')
print('Extracted original pure folio/page helpers')
