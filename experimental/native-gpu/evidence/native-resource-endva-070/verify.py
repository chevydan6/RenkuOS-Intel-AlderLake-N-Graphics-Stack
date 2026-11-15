#!/usr/bin/env python3
"""Check completed N100 draws at the last prebound resource page."""
import json
from pathlib import Path

root = Path(__file__).parent
sequences = set()
for name, direct in (("direct.log", 1), ("fallback.log", 0)):
    lines = (root / name).read_text().splitlines()
    assert lines[-1] == "Native Vulkan N100 triangle: PASS shape=0 hash=28616d2d draws=4"
    events = [json.loads(line) for line in lines if line.startswith("{")]
    assert len(events) == 4
    for event in events:
        assert event["event"] == "vk_queue_submit"
        assert event["version"] == (15 if direct else 7)
        assert event["direct_target"] == direct
        assert event["ioctl"] == event["write_ioctl"] == event["fault"] == 0
        assert event["sequence"] == event["observed"] > 0
        assert event["sequence"] not in sequences
        assert event["checked"] == event["matched"] == 777
        assert event["pixel_hash"] == "28616d2d"
        assert int(event["vertex_gpu_va"], 16) == 0x44c000
        assert int(event["image_gpu_va"], 16) == 0x44e000
        assert int(event["shader_gpu_va"], 16) == 0x44f000
        sequences.add(event["sequence"])
print("N100 final resource page: PASS (8 draws, 2 paths)")
