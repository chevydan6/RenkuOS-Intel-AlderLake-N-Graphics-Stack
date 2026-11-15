#!/usr/bin/env python3
"""Verify the saved VM-only UBO lowering experiment."""
from hashlib import sha256
from pathlib import Path

root = Path(__file__).parent
translated = (root / "translated-nir.log").read_text()
lowered = (root / "lowered-nir-and-result.log").read_text()
known = (root / "known-control.log").read_text()
bti1 = (root / "lowered-bti1-nir-and-result.log").read_text()
known_bti1 = (root / "known-bti1-control.log").read_text()
assert "@vulkan_resource_index" in translated
assert "@load_vulkan_descriptor" in translated
assert "@load_deref" in translated
assert "compile=-5" in translated
assert "@load_ubo" in lowered
assert "@vulkan_resource_index" not in lowered
assert "@load_vulkan_descriptor" not in lowered
assert "compile=0 error= vs_bytes=128 fs_bytes=336" in lowered
assert "fs_scratch=0 fs_relocs=0" in lowered
assert "compile=0 error= vs_bytes=128 fs_bytes=208" in known
assert "32x4   %9 = @load_ubo" in bti1
assert "32x2   %2 = load_const (0x00000001, 0x00000000)" in bti1
assert "compile=0 error= vs_bytes=128 fs_bytes=336" in bti1
assert "fs_scratch=0 fs_relocs=0" in bti1
assert "compile=0 error= vs_bytes=128 fs_bytes=208" in known_bti1
assert sha256((root / "lowered-fragment.bin").read_bytes()).hexdigest() == (
    "22f0a4732dcb0e3ba490efa731002df2bb4767a517e9cd80af325698cf30f646")
assert sha256((root / "lowered-bti1-fragment.bin").read_bytes()).hexdigest() == (
    "03179b37bcd77cf5399f298622c91e9a46d55dc8f79e1a518fb85fc729636f92")
print("VM UBO lowering: PASS (BTI 1, 336-byte Intel FS; known shader unchanged)")
