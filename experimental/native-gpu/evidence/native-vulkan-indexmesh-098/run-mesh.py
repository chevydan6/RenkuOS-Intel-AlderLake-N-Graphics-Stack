#!/usr/bin/env python3
"""Targeted N100 mesh submission, readback, and admission regression."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess


HERE = Path(__file__).resolve().parent
PRIOR = HERE.parent / "native-vulkan-draw9-087"
BUNDLE = "/boot/home/native-indexmesh-098"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")


def run(size, mode):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    capture = f"{BUNDLE}/capture-{size}-{mode}.bin"
    setting = f"RENKU_VK_CAPTURE_PIXELS={capture} " if mode != "indexmeshbad" else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 {setting}"
               f"timeout 20 {BUNDLE}/triangle_probe 0 original 1 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{")]
    draws = [e for e in events if e.get("event") == "vk_queue_submit"]
    assert len(draws) == 1 and draws[0]["version"] == 25
    draw = draws[0]
    assert draw["index_count"] == draw["vertex_count"] == 9
    assert draw["pixel_oracle"] == draw["checked"] == draw["matched"] == 0
    if mode == "indexmeshbad":
        assert result.returncode == 26 and draw["ioctl"] != 0
        assert draw["stage"] == draw["sequence"] == 0
        return result.stdout + result.stderr
    assert result.returncode == 0 and draw["ioctl"] == draw["fault"] == 0
    assert draw["stage"] == 12 and draw["sequence"] == draw["observed"] > 0
    if mode == "indexmeshclear":
        clears = [e for e in events if e.get("event") == "vk_clear_write"]
        assert len(clears) == 1 and clears[0]["color"] == "ff336699"
        assert clears[0]["status"] == 0
    output = HERE / f"capture-{size}-{mode}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    wanted = (PRIOR / f"draw9-{size}.bin").read_bytes()
    if mode == "indexmeshclear":
        wanted = b"".join(struct.pack("<I",0xff336699 if pixel==0x19537bad else pixel)
                          for pixel in struct.unpack(f"<{size*size}I",wanted))
    assert output.read_bytes() == wanted
    print(f"N100 {size} {mode}: full-image match "
          f"SHA-256 {hashlib.sha256(wanted).hexdigest()}", flush=True)
    return result.stdout + result.stderr


if __name__ == "__main__":
    cases = ((32,"indexmesh"),(64,"indexmesh"),
             (32,"indexmeshclear"),(32,"indexmeshbad"))
    lines = [f"size={size} mode={mode}\n{run(size,mode).rstrip()}"
             for size,mode in cases]
    (HERE / "hardware.log").write_text("\n".join(lines)+"\n")
