#!/usr/bin/env python3
"""Verify LOAD across separate Vulkan queue submissions on the physical N100."""
import hashlib
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-loadsubmit-093"
LIBRARIES = ("/boot/home/native-loadpass-091:/boot/home/native-xxhash-092:"
             "/boot/home/native-ubo-076:/boot/home/config/lib:/boot/system/lib")
EXPECTED = {
    32: ("225b1ba1", "893f4100ba027396244478779150c0c1e44aad4f68d2cf35cfe3f7f01d00077d"),
    64: ("bb1f873b", "b981f708c614a79c255941d9ed7f9f31d30653875c48e8db761c1d673093c575"),
}


def run(size, iteration):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    capture = f"{BUNDLE}/capture-{size}.bin"
    command = (f"LIBRARY_PATH={LIBRARIES} {target}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={capture} timeout 20 "
               f"{BUNDLE}/triangle_probe 0 original 2 loadsubmit")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    assert result.returncode == 0 and len(events) == 2, result
    assert [e["version"] for e in events] == [15, 19], events
    assert events[0]["sequence"] < events[1]["sequence"], events
    assert events[0]["image_bo"] == events[1]["image_bo"], events
    assert all(e["ioctl"] == e["write_ioctl"] == e["fault"] == 0
               and e["stage"] == 12 and e["draw_count"] == e["gpu_draws"] == 1
               and e["sequence"] == e["observed"] > 0
               and e["checked"] == e["matched"] > 0 for e in events), events
    assert events[-1]["pixel_hash"] == EXPECTED[size][0], events
    output = HERE / f"capture-{size}-{iteration}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{capture}", str(output)],
                   check=True, timeout=20)
    assert output.stat().st_size == size * size * 4
    assert hashlib.sha256(output.read_bytes()).hexdigest() == EXPECTED[size][1]
    return (f"size={size} iteration={iteration} rc=0\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}")


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for iteration in range(3):
            lines.append(run(size, iteration))
            print(f"N100 {size} cross-submit LOAD iteration {iteration}: PASS", flush=True)
    (HERE / "hardware.log").write_text("\n".join(lines) + "\n")
