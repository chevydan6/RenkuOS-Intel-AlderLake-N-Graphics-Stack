#!/usr/bin/env python3
"""Verify native Vulkan CLEAR plus GPU draw against saved N100 pixels."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess


HERE = Path(__file__).resolve().parent
PRIOR = HERE.parent / "native-vulkan-indexpair-095"
BUNDLE = "/boot/home/native-clearpass-097"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
CLEAR = 0xff336699
POISON = 0x19537bad


def run(size):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    remote = f"{BUNDLE}/capture-{size}-clear.bin"
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={remote} timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 1 clearpass")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{")]
    clears = [e for e in events if e.get("event") == "vk_clear_write"]
    draws = [e for e in events if e.get("event") == "vk_queue_submit"]
    assert result.returncode == 0 and len(clears) == len(draws) == 1, result
    assert clears[0]["status"] == 0 and clears[0]["color"] == f"{CLEAR:08x}"
    assert clears[0]["pages"] == (4 if size == 64 else 1)
    assert draws[0]["version"] == 19 and draws[0]["stage"] == 12
    assert draws[0]["ioctl"] == draws[0]["fault"] == 0
    assert draws[0]["sequence"] == draws[0]["observed"] > 0
    assert draws[0]["checked"] == draws[0]["matched"] > 0
    output = HERE / f"capture-{size}-clear.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{remote}", str(output)],
                   check=True, timeout=20)
    previous = (PRIOR / f"capture-{size}-1-0.bin").read_bytes()
    wanted = b"".join(struct.pack("<I", CLEAR if pixel == POISON else pixel)
                      for pixel in struct.unpack(f"<{size*size}I", previous))
    assert output.read_bytes() == wanted
    print(f"N100 {size}: CLEAR and GPU draw match full-image oracle, "
          f"SHA-256 {hashlib.sha256(wanted).hexdigest()}", flush=True)
    return result.stdout + result.stderr


if __name__ == "__main__":
    lines = [f"size={size}\n{run(size).rstrip()}" for size in (32, 64)]
    (HERE / "clear-hardware.log").write_text("\n".join(lines) + "\n")
