#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[1]
subprocess.run(['python3',str(root/'verify-upstream.py')],check=True)
source = (root/'upstream/lib/scatterlist.c').read_text()
text = '/* SPDX-License-Identifier: GPL-2.0-or-later */\n#include <linux/scatterlist.h>\n'
for signature in ['struct scatterlist *sg_next(', 'void sg_init_table(']:
    start = source.index(signature)
    end = source.index('\n}', start)+2
    text += source[start:end]+'\n'
(root/'build').mkdir(exist_ok=True)
(root/'build/scatterlist_slice.c').write_text(text)
