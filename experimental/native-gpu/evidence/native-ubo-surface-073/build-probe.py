#!/usr/bin/env python3
"""Compile the CPU packet probe with the pinned Mesa 26.3 build flags."""
import json
from pathlib import Path
import shlex
import subprocess
import sys

if len(sys.argv) != 3:
    raise SystemExit("usage: build-probe.py MESA_WORKSPACE OUTPUT_BINARY")
workspace = Path(sys.argv[1]).resolve()
build = workspace / "build"
source = Path(__file__).with_name("emit-surface.c").resolve()
output = Path(sys.argv[2]).resolve()
commands = json.loads((build / "compile_commands.json").read_text())
entry = next(c for c in commands if c["file"].endswith("/native_rcs_shaders.c"))
args = shlex.split(entry["command"])
args = args[:args.index("-MD")]
args += ["-I" + str(build / "src/intel/genxml"),
         "-I" + str(workspace / "mesa-26.3.0/src/intel/genxml"),
         "-o", str(output), str(source)]
subprocess.run(args, cwd=build, check=True)
subprocess.run([str(output)], check=True)
