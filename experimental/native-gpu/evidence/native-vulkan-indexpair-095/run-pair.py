#!/usr/bin/env python3
"""Compare preserving indexed pair with a nonindexed N100 hardware reference."""
import hashlib
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-indexpair-095"
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
    capture_setting = f"RENKU_VK_CAPTURE_PIXELS={capture} " if direct else ""
    command = (f"LIBRARY_PATH={LIBRARIES} {target}{direct_setting}"
               f"RENKU_VK_TRACE=1 {capture_setting}timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 2 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    if not direct:
        assert result.returncode == 26 and not events, result
        return result.stdout + result.stderr, None
    versions = [22, 23] if mode == "indexpair" else [20]
    assert result.returncode == 0 and [e["version"] for e in events] == versions, result
    assert all(e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
               and e["stage"] == 12 and e["sequence"] == e["observed"] > 0
               and e["checked"] == e["matched"] > 0
               for e in events), events
    assert events[-1]["pixel_hash"] == EXPECTED[size][0], events
    if mode == "indexpair":
        assert events[0]["pixel_hash"] == ("28616d2d" if size == 32 else "8aaee2f5")
        assert events[0]["sequence"] < events[1]["sequence"]
        assert events[0]["image_bo"] == events[1]["image_bo"]
        assert events[0]["index_bo"] == events[1]["index_bo"]
        assert [e["index_offset"] for e in events] == [0, 6]
        assert all(e["gpu_draws"] == 1 for e in events)
    else:
        assert events[0]["gpu_draws"] == 2
    output = HERE / f"capture-{size}-{mode}-{iteration}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    assert output.stat().st_size == size * size * 4
    assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][1]
    return result.stdout + result.stderr, output


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for iteration in range(3):
            indexed, image = run(size, "indexpair", iteration)
            reference, expected = run(size, "indexpairref", iteration)
            assert image.read_bytes() == expected.read_bytes()
            lines.append(f"size={size} iteration={iteration} indexed\n{indexed.rstrip()}")
            lines.append(f"size={size} iteration={iteration} reference\n{reference.rstrip()}")
            print(f"N100 {size} indexed pair/reference iteration {iteration}: PASS", flush=True)
    rejected, _ = run(32, "indexpair", 0, direct=False)
    lines.append("direct target disabled\n" + rejected.rstrip())
    print("N100 indexed pair without direct target: rejected before submit", flush=True)
    (HERE / "pair-hardware.log").write_text("\n".join(lines) + "\n")
