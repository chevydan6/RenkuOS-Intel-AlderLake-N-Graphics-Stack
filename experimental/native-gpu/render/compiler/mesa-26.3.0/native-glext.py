#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Derive the GL constants needed by the selected native Vulkan compiler."""
import hashlib
from pathlib import Path
import re
import sys

GLHEADER_SHA = "bed967e399e9f0fde4d2821773c5280a122fecc8bfedeaf1fd53a02d75f1a057"
GLEXT_SHA = "86c4c01c82d2090c38551755851c0efda3acf18f2cce812e7a6ca93347110743"
TOKEN = re.compile(r"\bGL_[A-Za-z0-9_]+\b")
DEFINITION = re.compile(r"^#define GL_[A-Za-z0-9_]+(?:\s|$)")


def checked(path: Path, expected: str) -> str:
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f"unexpected source: {path}")
    return data.decode()


def derive(glheader: Path, glext: Path, file_list: Path, output: Path):
    header = checked(glheader, GLHEADER_SHA)
    extension = checked(glext, GLEXT_SHA)
    macro_lines = [line for line in extension.splitlines() if DEFINITION.match(line)]
    macros = {line.split()[1]: line for line in macro_lines}
    needed = set()
    for filename in file_list.read_text().splitlines():
        path = Path(filename)
        if path.name in ("glext.h", "gl_ext_constants.h"):
            continue
        needed.update(TOKEN.findall(path.read_text(errors="replace")))
    needed.intersection_update(macros)
    while True:
        references = set()
        for name in needed:
            references.update(TOKEN.findall(macros[name]))
        expanded = needed | (references & macros.keys())
        if expanded == needed:
            break
        needed = expanded

    constants = ("/*\n"
                 "** Copyright 2013-2020 The Khronos Group Inc.\n"
                 "** SPDX-License-Identifier: MIT\n"
                 "** Selected from Mesa 26.3.0's Khronos GL/glext.h.\n"
                 "*/\n"
                 "#ifndef RENKU_GL_EXT_CONSTANTS_H\n"
                 "#define RENKU_GL_EXT_CONSTANTS_H\n"
                 + "\n".join(line for line in macro_lines
                             if line.split()[1] in needed) + "\n#endif\n")
    header = header.replace("#define GL_GLEXT_PROTOTYPES\n",
                            "#define GL_GLEXT_PROTOTYPES\n#define GL_GLEXT_LEGACY\n")
    header = header.replace('#include "GL/glext.h"\n', "")
    header = header.replace('#endif /* defined(_WIN32) && !defined(__CYGWIN__) */\n',
                            '#endif /* defined(_WIN32) && !defined(__CYGWIN__) */\n'
                            '#include "gl_ext_constants.h"\n')
    header = header.replace("typedef GLuint64 GLbitfield64;",
                            "typedef unsigned long long GLuint64;\n"
                            "typedef GLuint64 GLbitfield64;")
    destination = output / "util"
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "glheader.h").write_text(header)
    (destination / "gl_ext_constants.h").write_text(constants)
    print(f"selected {len(needed)} GL constants ({len(constants.encode())} bytes)")


if __name__ == "__main__":
    if len(sys.argv) != 5:
        raise SystemExit("usage: native-glext.py NATIVE_GLHEADER MESA_GLEXT "
                         "COMPILER_FILES OUT_ROOT")
    derive(*(Path(arg) for arg in sys.argv[1:]))
