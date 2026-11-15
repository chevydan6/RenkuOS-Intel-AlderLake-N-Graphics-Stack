#!/usr/bin/env python3
"""Verify four-triangle preserving batches and fallback on the physical N100."""
import json
from pathlib import Path
import subprocess


HERE = Path(__file__).resolve().parent
BUNDLE = "/boot/home/native-pairpreserve-088"
LIBRARIES = (BUNDLE + ":/boot/home/native-blake3-080:/boot/home/native-ubo-076:"
             "/boot/home/config/lib:/boot/system/lib")
EXPECTED = {32: "5bf758b9", 64: "61162dae"}


def run(size, mode, batch):
    target = "RENKU_VK_TARGET_64=1 " if size == 64 else ""
    setting = "" if batch else "RENKU_VK_NATIVE_BATCH=0 "
    command = (f"LIBRARY_PATH={LIBRARIES} {target}{setting}RENKU_VK_TRACE=1 "
               f"RENKU_VK_CAPTURE_PIXELS={BUNDLE}/capture.bin "
               f"timeout 20 {BUNDLE}/triangle_probe 0 original 4 {mode}")
    result = subprocess.run(["ssh", "renkuos-n100", command],
                            capture_output=True, text=True, timeout=30)
    events = [json.loads(line) for line in result.stderr.splitlines()
              if line.startswith("{") and '"event":"vk_queue_submit"' in line]
    versions = [20, 21] if batch else [15, 19, 19, 19]
    assert result.returncode == 0 and len(events) == len(versions), result
    assert [event["version"] for event in events] == versions, events
    assert f"hash={EXPECTED[size]}" in result.stdout, result.stdout
    assert f"calls={1 if mode == 'draw12' else 4}" in result.stdout
    assert all(event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
               and event["sequence"] == event["observed"] > 0
               and event["matched"] == event["checked"] > 0
               for event in events), events
    assert events[-1]["pixel_hash"] == EXPECTED[size]
    output = HERE / f"{mode}-{size}-{'batch' if batch else 'fallback'}.bin"
    subprocess.run(["scp", "-q", f"renkuos-n100:{BUNDLE}/capture.bin",
                    str(output)], check=True, timeout=20)
    assert output.read_bytes() == (HERE / f"baseline{size}.bin").read_bytes()
    return (f"size={size} mode={mode} batch={int(batch)} rc={result.returncode}\n"
            f"{result.stdout.strip()}\n{result.stderr.strip()}")


if __name__ == "__main__":
    lines = []
    for size in (32, 64):
        for mode, batch in (("draw12", True), ("draw3x4", True),
                            ("draw12", False)):
            lines.append(run(size, mode, batch))
            print(f"N100 preserving pair: PASS size={size} mode={mode} "
                  f"batch={int(batch)}", flush=True)
    (HERE / "hardware.log").write_text("\n".join(lines) + "\n")
