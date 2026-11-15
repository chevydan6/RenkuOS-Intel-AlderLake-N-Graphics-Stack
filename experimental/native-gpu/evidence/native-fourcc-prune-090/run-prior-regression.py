#!/usr/bin/env python3
"""Replay the physical checkpoint 084 matrix with the compiler candidate."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


HERE = Path(__file__).resolve().parent
PRIOR = HERE.parent / "native-gpubatch64-084"
SCRIPTS = [PRIOR / "regression" / f"run-{name}.py"
           for name in ("regression", "physical", "green", "arena",
                        "supplemental")]
SCRIPTS += [PRIOR / f"run-{name}-offset.py"
            for name in ("image", "memory")]
DESTINATION = HERE / "regression"
DESTINATION.mkdir(exist_ok=True)

with tempfile.TemporaryDirectory(prefix="renku-fourcc-regression-") as folder:
    temp = Path(folder)
    for source in SCRIPTS:
        original = source.read_text()
        changed = original.replace("/boot/home/native-gpubatch64-084",
                                   "/boot/home/native-pairpreserve-088")
        changed = changed.replace("LIBRARY_PATH=",
                                  "LIBRARY_PATH=/boot/home/native-fourcc-090:")
        assert changed != original and "LIBRARY_PATH=" in changed, source
        script = temp / source.name
        script.write_text(changed)
        result = subprocess.run([sys.executable, str(script)],
                                capture_output=True, text=True, timeout=300)
        (DESTINATION / f"{source.stem}.stdout.log").write_text(result.stdout)
        (DESTINATION / f"{source.stem}.stderr.log").write_text(result.stderr)
        for log in temp.glob("*.log"):
            shutil.copy2(log, DESTINATION / log.name)
            log.unlink()
        if result.returncode:
            raise RuntimeError(f"{source.name} failed: {result.stderr[-1200:]}")
        print(result.stdout.strip().splitlines()[-1], flush=True)
