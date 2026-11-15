#!/usr/bin/env python3
"""Verify one six-index N100 draw against the saved two-triangle oracle."""
import hashlib
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-indexsix-096"
LIBRARIES = (BUNDLE + ":/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
EXPECTED = {
    32: ("261a8285", "25a9432f9a19c4f68a74a6b02b1679f85990f5257af9b82d155097509e5efd61"),
    64: ("554d3ec5", "3bf868304fc2fdd05c9e50f2ef93ce3ccf47f5bb011d2c3c8cc56e01e7e24409"),
}


def run(size, mode, iteration, direct=True):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    direct_setting = "" if direct else "RENKU_VK_DIRECT_TARGET=0 "
    capture = f"{BUNDLE}/capture-{size}-{mode}.bin"
    capture_setting = f"RENKU_VK_CAPTURE_PIXELS={capture} " if direct and mode == "indexsix" else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}{direct_setting}"
               f"RENKU_VK_TRACE=1 {capture_setting}timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original "
               f"{'2' if mode == 'indexpairref' else '1'} {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    if not direct:
        assert result.returncode == 26 and not events, result
        return result.stdout + result.stderr, None
    if mode == "indexsix":
        assert result.returncode == 0 and len(events) == 1, result
        e = events[0]
        assert e["version"] == 24 and e["index_count"] == 6
        assert e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
        assert e["stage"] == 12 and e["sequence"] == e["observed"] > 0
        assert e["checked"] == e["matched"] > 0
        assert e["pixel_hash"] == EXPECTED[size][0]
        output = HERE / f"capture-{size}-indexsix-{iteration}.bin"
        subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                       check=True, timeout=20)
        assert output.stat().st_size == size * size * 4
        assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][1]
        reference = (HERE.parent / "native-vulkan-indexpair-095" /
                     f"capture-{size}-indexpairref-0.bin")
        assert output.read_bytes() == reference.read_bytes()
        return result.stdout + result.stderr, output
    assert mode == "indexpairref"
    assert result.returncode == 0 and len(events) == 1, result
    assert events[0]["version"] == 20 and events[0]["pixel_hash"] == EXPECTED[size][0]
    return result.stdout + result.stderr, None


def reject(size, mode, expected_code, owner_event):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 1 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    assert result.returncode == expected_code and len(events) == owner_event, result
    if owner_event:
        assert events[0]["version"] == 24 and events[0]["stage"] == 0
        assert events[0]["sequence"] == 0 and events[0]["ioctl"] != 0
    return result.stdout + result.stderr


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for iteration in range(3):
            result, _ = run(size, "indexsix", iteration)
            lines.append(f"size={size} iteration={iteration} six-index\n{result.rstrip()}")
            print(f"N100 {size} six-index iteration {iteration}: PASS", flush=True)
        reference, _ = run(size, "indexpairref", 0)
        lines.append(f"size={size} reference\n{reference.rstrip()}")
        for mode, code, events in (("indexsixbad", 26, 1),
                                   ("indexsixpage", 25, 0)):
            result = reject(size, mode, code, events)
            lines.append(f"size={size} {mode}\n{result.rstrip()}")
            print(f"N100 {size} {mode}: rejected", flush=True)
    result, _ = run(32, "indexsix", 0, direct=False)
    lines.append("direct target disabled\n" + result.rstrip())
    (HERE / "six-hardware.log").write_text("\n".join(lines) + "\n")
