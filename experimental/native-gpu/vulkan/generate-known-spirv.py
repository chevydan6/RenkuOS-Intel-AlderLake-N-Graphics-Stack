#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate Vulkan shader fixture words from the validated assembly sources."""
import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ASSEMBLY = HERE.parent / "render/compiler/mesa-26.3.0/spirv"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spirv_tools", type=Path)
    args = parser.parse_args()
    tool = args.spirv_tools.resolve()
    text = ["// SPDX-License-Identifier: MIT", "// Generated from the pinned Vulkan 1.0 SPIR-V assembly fixtures.",
            "#ifndef RENKU_NATIVE_KNOWN_SPIRV_H", "#define RENKU_NATIVE_KNOWN_SPIRV_H",
            "#include <stdint.h>", "namespace RenkuKnownSpirv {"]
    with tempfile.TemporaryDirectory(prefix="renku-spirv-") as temp:
        for stage, name in (("vert", "Vertex"), ("frag", "Fragment")):
            source = ASSEMBLY / f"triangle.{stage}.spvasm"
            output = Path(temp) / f"triangle.{stage}.spv"
            subprocess.run([str(tool / "spirv-as"), "--target-env", "vulkan1.0",
                            str(source), "-o", str(output)], check=True)
            subprocess.run([str(tool / "spirv-val"), "--target-env", "vulkan1.0",
                            str(output)], check=True)
            data = output.read_bytes()
            words = struct.unpack("<" + "I" * (len(data) // 4), data)
            text.append(f"// SHA-256 {hashlib.sha256(data).hexdigest()}")
            text.append(f"static constexpr uint32_t {name}[] = {{")
            text.extend(" " + ", ".join(f"0x{word:08x}U" for word in words[i:i+6]) + ","
                        for i in range(0, len(words), 6))
            text.append("};")
    text.extend(("}", "#endif", ""))
    (HERE / "KnownSpirv.h").write_text("\n".join(text))

if __name__ == "__main__":
    main()
