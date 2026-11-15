#!/usr/bin/env python3
"""Check the saved UBO boundary and physical rendering evidence."""
from pathlib import Path
import subprocess

root = Path(__file__).parent
old = (root / "old-ubo-repro.log").read_text()
assert "validate=0 error=" in old and "exit=137" in old
candidate = (root / "candidate-ubo-vm.log").read_text()
physical = (root / "ubo-rejection-n100.log").read_text()
for log in (candidate, physical):
    assert "validate=0 error=" in log
    assert "compile=-5 error=uniform buffers need native descriptor lowering" in log
known = (root / "candidate-known-vm.log").read_text()
assert "compile=0 error= vs_bytes=128 fs_bytes=208" in known
first = (root / "first-draw-n100.log").read_text()
assert "Native Vulkan N100 triangle: PASS shape=0 hash=28616d2d draws=4" in first
subprocess.run(["python3", str(root / "regression/verify.py")], check=True)
print("UBO compiler boundary: PASS (fast rejection; 206 N100 regression draws)")
