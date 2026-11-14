#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify the A/B/A/B hardware traces and print their ioctl timings."""
import json
from pathlib import Path
from statistics import median

HERE = Path(__file__).resolve().parent
for name in ("baseline.log", "optimized.log", "baseline-return.log",
             "optimized-return.log"):
    lines = (HERE / name).read_text().splitlines()
    events = [json.loads(line) for line in lines if line.startswith("{")]
    assert len(events) == 128, (name, len(events))
    assert all(e["ioctl"] == 0 and e["fault"] == 0
               and e["version"] == 12 and e["pixel_hash"] == "7b2f62f5"
               and e["checked"] == e["matched"] == 3476
               and e["observed"] == e["sequence"] for e in events), name
    values = [e["render_ioctl_us"] for e in events]
    print(f"{name}: draws={len(values)} min={min(values)} "
          f"median={median(values)} max={max(values)} us")
