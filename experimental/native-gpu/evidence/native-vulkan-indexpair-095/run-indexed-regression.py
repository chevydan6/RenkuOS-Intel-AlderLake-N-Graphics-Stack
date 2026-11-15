#!/usr/bin/env python3
"""Replay checkpoint-094 index fetch, update, and rejection on the N100."""
import hashlib
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-indexpair-095"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
EXPECTED = {
    32: (("28616d2d", "d078ce6ad13cc5e13e85f660b0ea53a2963a47ad619297d7796f82e1f82627eb"),
         ("459d92c5", "fe182d4ba6c6fa704fd4110a834ed74508614b2fc44f667f397fc9eff92ebcfb")),
    64: (("8aaee2f5", "c3cd98385b4fc4dc40f92bec112697c8ecaabe670c029db25c6f18217591882d"),
         ("5fdea115", "f7b22aba25b8688ccfe24dcc38707de308242d27401861888dd0404c09f58d65")),
}


def run(size, count, iteration, mode="indexed"):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    capture = f"{BUNDLE}/capture-indexed-{size}.bin"
    capture_setting = (f"RENKU_VK_CAPTURE_PIXELS={capture} "
                       if mode == "indexed" else "")
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"{capture_setting}timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original {count} {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    if mode != "indexed":
        assert result.returncode == 0 and len(events) == 1, result
        assert events[0]["version"] == 22 and events[0]["ioctl"] == -1
        assert events[0]["stage"] == events[0]["sequence"] == 0
        assert events[0]["fault"] == 0 and events[0]["checked"] == 0
        return result.stdout + result.stderr
    assert result.returncode == 0 and len(events) == count, result
    assert all(e["version"] == 22 and e["stage"] == 12
               and e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
               and e["gpu_draws"] == e["draw_count"] == 1
               and e["sequence"] == e["observed"] > 0
               and e["checked"] == e["matched"] > 0
               and e["index_bo"] != e["vertex_bo"]
               and e["index_bo"] != e["image_bo"]
               and e["pixel_hash"] == EXPECTED[size][j][0]
               for j, e in enumerate(events)), events
    if count == 2:
        assert events[0]["sequence"] < events[1]["sequence"]
        assert events[0]["index_bo"] == events[1]["index_bo"]
        assert events[0]["image_bo"] == events[1]["image_bo"]
    output = HERE / f"capture-{size}-{count}-{iteration}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    assert output.stat().st_size == size * size * 4
    assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][count - 1][1]
    return result.stdout + result.stderr


def reference(size):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    capture = f"{BUNDLE}/capture-reference-{size}.bin"
    command = (f"LIBRARY_PATH={LIBRARIES} {target}"
               f"RENKU_VK_CAPTURE_PIXELS={capture} timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 1")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result
    output = HERE / f"capture-{size}-reference.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][0][1]
    return result.stdout + result.stderr


def without_direct_target():
    command = (f"LIBRARY_PATH={LIBRARIES} RENKU_VK_DIRECT_TARGET=0 "
               f"RENKU_VK_TRACE=1 timeout 20 {BUNDLE}/triangle_probe "
               "0 original 1 indexed")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    assert result.returncode == 26 and '"event":"vk_queue_submit"' not in result.stderr
    return result.stdout + result.stderr


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for count in (1, 2):
            for iteration in range(3):
                lines.append(f"size={size} count={count} iteration={iteration}\n"
                             + run(size, count, iteration))
                print(f"N100 {size} indexed count={count} iteration={iteration}: PASS",
                      flush=True)
        assert (HERE / f"capture-{size}-1-0.bin").read_bytes() != (
            HERE / f"capture-{size}-2-0.bin").read_bytes()
        lines.append(f"size={size} sequential reference\n" + reference(size))
        assert (HERE / f"capture-{size}-1-0.bin").read_bytes() == (
            HERE / f"capture-{size}-reference.bin").read_bytes()
        for mode in ("indexbad", "indexalias"):
            lines.append(f"size={size} {mode}\n" + run(size, 1, 0, mode))
            print(f"N100 {size} {mode}: rejected before draw", flush=True)
    lines.append("direct target disabled\n" + without_direct_target())
    print("N100 indexed without direct target: rejected before submit", flush=True)
    (HERE / "indexed-hardware.log").write_text(
        "\n".join(line.rstrip() for line in lines) + "\n")
