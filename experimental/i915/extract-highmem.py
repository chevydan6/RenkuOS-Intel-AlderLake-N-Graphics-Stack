#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Extract complete original direct-map atomic helpers, without Linux VFS."""
import hashlib
import json
from pathlib import Path
root = Path(__file__).resolve().parent
entry = next(f for f in json.loads((root/'upstream-manifest.json').read_text())['files']
             if f['path'] == 'linux/highmem-internal.h')
data = (root/'upstream/linux/highmem-internal.h').read_bytes()
assert hashlib.sha256(data).hexdigest() == entry['sha256']
source = data.decode()
branch = source.index('#else /* CONFIG_HIGHMEM */')
functions = []
for signature in ['static inline void *kmap_atomic(struct page *page)',
                  'static inline void __kunmap_atomic(const void *addr)',
                  'static inline void *kmap_local_page(struct page *page)',
                  'static inline void *kmap_local_folio(struct folio *folio, size_t offset)',
                  'static inline void __kunmap_local(const void *addr)']:
    start = source.index(signature, branch)
    end = source.index('\n}', start) + 2
    functions.append(source[start:end])
start = source.index('#define kunmap_atomic(__addr)')
end = source.index('} while (0)', start) + len('} while (0)')
functions.append(source[start:end])
start = source.index('#define kunmap_local(__addr)')
end = source.index('} while (0)', start) + len('} while (0)')
functions.append(source[start:end])
(root/'build').mkdir(exist_ok=True)
(root/'build/highmem_atomic.h').write_text('/* SPDX-License-Identifier: GPL-2.0 */\n'
    '/* Complete original v6.6 direct-map kmap/kunmap_atomic, non-RT only. */\n'
    + '\n\n'.join(functions) + '\n')
(root/'build/highmem_atomic.json').write_text(json.dumps({'source': entry,
    'functions': [{'signature': f.splitlines()[0], 'sha256': hashlib.sha256(f.encode()).hexdigest()}
                  for f in functions], 'completeHighmemPort': False}, indent=2)+'\n')
print('Extracted complete original direct-map atomic kmap helpers')
