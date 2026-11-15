#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Derive a native compiler glheader without unused GLES declarations."""
import hashlib
from pathlib import Path
import sys

EXPECTED = "def3c9be62583645c6ecb0f9579414a75aed664fca4fa2d20049f3a54ba7f184"
START = '#include "GLES3/gl3.h"\n'
END = '#include "GLES/glext.h"\n'

def derive(source: Path, destination: Path):
    data = source.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise ValueError("unexpected Mesa 26.3.0 glheader.h")
    text = data.decode()
    assert text.count(START) == text.count(END) == 1
    begin = text.index(START)
    end = text.index(END, begin) + len(END)
    text = (text[:begin] +
            "/* Native Vulkan compiler needs these two GLES enum values, not\n"
            " * the full GLES declarations. Values: Khronos GLES2/gl2ext.h. */\n"
            "#define GL_SAMPLER_EXTERNAL_OES 0x8D66\n"
            "#define GL_SAMPLER_EXTERNAL_2D_Y2Y_EXT 0x8BE7\n" +
            text[end:])
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(text)
    print(f"derived {destination} ({len(data)} -> {destination.stat().st_size} bytes)")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: native-glheader.py MESA_GLHEADER OUT_ROOT/util/glheader.h")
    derive(Path(sys.argv[1]), Path(sys.argv[2]))
