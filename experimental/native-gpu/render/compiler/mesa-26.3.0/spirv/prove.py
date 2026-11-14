#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Assemble Vulkan SPIR-V and prove Mesa 26.3.0 emits the tested N100 code."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

HERE = Path(__file__).resolve().parent
ARTIFACTS = HERE.parent / "artifacts"

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def run(args, cwd=None, env=None):
    subprocess.run(args, cwd=cwd, env=env, check=True)

def require(condition, message):
    if not condition:
        raise ValueError(message)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("compiler_workspace", type=Path)
    parser.add_argument("spirv_tools", type=Path, help="directory with spirv-as and spirv-val")
    parser.add_argument("output", type=Path, help="new, nonexistent directory")
    args = parser.parse_args()
    root, tools, work = (p.resolve() for p in
                         (args.compiler_workspace, args.spirv_tools, args.output))
    require((root / "mesa-26.3.0/VERSION").read_text().strip() == "26.3.0",
            "compiler workspace is not Mesa 26.3.0")
    work.mkdir(parents=True, exist_ok=False)
    spv = {}
    for stage in ("vert", "frag"):
        source = HERE / f"triangle.{stage}.spvasm"
        spv[stage] = work / f"triangle.{stage}.spv"
        run([str(tools / "spirv-as"), "--target-env", "vulkan1.2",
             str(source), "-o", str(spv[stage])])
        run([str(tools / "spirv-val"), "--target-env", "vulkan1.2",
             str(spv[stage])])

    source = HERE / "compile_spirv.c"
    (work / "source_identity.h").write_text(
        f'#define NATIVE_RCS_SOURCE_SHA256 "{digest(source)}"\n')
    build = root / "build"
    commands = json.loads((build / "compile_commands.json").read_text())
    entry = next(c for c in commands if c["file"].endswith("/native_rcs_shaders.c"))
    command = shlex.split(entry["command"])
    command = command[:command.index("-MD")]
    obj = work / "compile_spirv.o"
    # The Mesa tree also has source_identity.h; put this probe's header first.
    run(command[:1] + ["-I" + str(work)] + command[1:] +
        ["-o", str(obj), "-c", str(source)], cwd=build)

    ninja = (build / "build.ninja").read_text()
    start = ninja.index("build src/intel/compiler/brw/native-rcs-shaders:")
    link = ninja.index(" LINK_ARGS = ", start)
    link_args = shlex.split(ninja[link:].splitlines()[0].split(" = ", 1)[1])
    executable = work / "compile_spirv"
    run(["c++", "-o", str(executable), str(obj)] + link_args, cwd=build)
    clean_env = {key: os.environ[key] for key in ("PATH", "LD_LIBRARY_PATH")
                 if key in os.environ}
    output = work / "artifacts"
    run([str(executable), str(output), str(spv["vert"]), str(spv["frag"])],
        env=clean_env)

    actual = json.loads((output / "metadata.json").read_text())
    expected = json.loads((ARTIFACTS / "metadata.json").read_text())
    require(actual.pop("harness_sha256") == digest(source),
            "SPIR-V harness source identity differs")
    expected.pop("harness_sha256")
    require(actual == expected,
            "SPIR-V shader metadata differs from tested direct NIR")
    for stage in ("vs", "fs"):
        require((output / f"{stage}.bin").read_bytes() ==
                (ARTIFACTS / f"{stage}.bin").read_bytes(),
                f"{stage} machine code differs from hardware-tested shader")
    proof = {
        "mesa_version": "26.3.0",
        "harness_sha256": digest(source),
        "vertex_spirv_sha256": digest(spv["vert"]),
        "fragment_spirv_sha256": digest(spv["frag"]),
        "vs_machine_code_sha256": digest(output / "vs.bin"),
        "fs_machine_code_sha256": digest(output / "fs.bin"),
        "same_machine_code_as_hardware_tested_direct_nir": True,
    }
    (work / "proof.json").write_text(json.dumps(proof, indent=2) + "\n")
    print("PASS: validated Vulkan SPIR-V -> Mesa NIR -> byte-identical N100 VS/FS")

if __name__ == "__main__":
    main()
