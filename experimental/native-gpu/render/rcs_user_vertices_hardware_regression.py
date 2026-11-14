#!/usr/bin/env python3
"""Repeat native RCS user-vertex draws and verify physical N100 pixel hashes."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import time

CLIENT = "/boot/home/native-rcs-userdraw-006/build/gpu_render_vertices"
OWNER = "/boot/system/non-packaged/add-ons/kernel/bus_managers/intel_memory_owner"
SHAPES = (
    ((4, 4, 28, 4, 28, 28), 777, "28616d2d"),
    ((5, 5, 27, 5, 16, 27), 758, "d40eb9c3"),
    ((4, 24, 16, 4, 28, 24), 766, "e24aed05"),
    ((8, 8, 24, 8, 16, 24), 760, "984ae945"),
)


def run(*args):
    return subprocess.run(args, text=True, capture_output=True, timeout=20)


def parse(output, resource=False, public_buffer=False, public_batch=False):
    kind = "resource" if resource else "user"
    version = 5 if public_batch else 4 if public_buffer else 3 if resource else 2
    success = ("RCS public batch hardware draw: PASS" if public_batch else
               "RCS public buffer hardware draw: PASS" if public_buffer else
               f"RCS {kind} vertex hardware draw: PASS")
    prefix = ("rcs_public_batch" if public_batch else "rcs_public_buffer" if public_buffer
              else f"rcs_{kind}_vertices")
    if success not in output:
        raise ValueError("client did not confirm the hardware draw")
    status = re.search(
        rf"{prefix} ioctl=0 status=0 version={version} stage=(\d+)"
        r" refusal=(\d+) sequence=(\d+)/(\d+)", output)
    image = re.search(r"pixels=(\d+)/(\d+) hash=([0-9a-fA-F]{8})", output)
    engine = re.search(r"engine=(\d+)/(\d+) csb=(\d+) fault=([0-9a-fA-F]{8})", output)
    if not status or not image or not engine:
        raise ValueError("missing completion, image, or engine evidence")
    stage, refusal, sequence, observed = map(int, status.groups())
    matched, checked = map(int, image.groups()[:2])
    engine_stage, engine_error, csb_error = map(int, engine.groups()[:3])
    if stage != 12 or refusal or sequence == 0 or sequence != observed:
        raise ValueError("submission or completion mismatch")
    if checked < 500 or matched != checked:
        raise ValueError("pixel verification mismatch")
    if engine_stage != 4 or engine_error or csb_error or int(engine.group(4), 16):
        raise ValueError("engine or fault report changed")
    return {"sequence": sequence, "checked_pixels": checked,
            "target_hash": image.group(3).lower()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rounds", type=int, default=16)
    parser.add_argument("--host", default="renkuos-n100")
    parser.add_argument("--client", default=CLIENT)
    parser.add_argument("--resource", action="store_true")
    parser.add_argument("--public-buffer", action="store_true")
    parser.add_argument("--public-batch", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.rounds <= 1000:
        parser.error("--rounds must be 1..1000")
    if sum((args.resource,args.public_buffer,args.public_batch))>1:
        parser.error("select at most one draw mode")
    args.output.mkdir(parents=True, exist_ok=False)
    meta = {"client": args.client, "owner": OWNER, "host": args.host,
            "resource": args.resource, "public_buffer": args.public_buffer,
            "public_batch": args.public_batch,
            "rounds": args.rounds, "shapes": SHAPES,
            "start_unix_ns": time.time_ns()}
    for label, path in (("client_sha256", args.client), ("owner_sha256", OWNER)):
        digest = run("ssh", args.host, "sha256sum", path)
        if digest.returncode:
            raise SystemExit(digest.stderr or f"could not hash {path}")
        meta[label] = digest.stdout.split()[0]
    (args.output / "metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    last_sequence = None
    with (args.output / "draws.jsonl").open("w") as records, \
         (args.output / "draws.raw.log").open("w") as raw:
        for round_index in range(args.rounds):
            for shape_index, (coords, expected_pixels, expected_hash) in enumerate(SHAPES):
                result = run("ssh", args.host, args.client,
                             *(("--resource",) if args.resource else
                               ("--batch",) if args.public_batch else ()),
                             *(str(x) for x in coords))
                raw.write(f"round={round_index} shape={shape_index} exit={result.returncode}\n")
                raw.write(result.stdout + result.stderr + "\n")
                raw.flush()
                try:
                    if result.returncode:
                        raise ValueError(f"client exited {result.returncode}")
                    record = parse(result.stdout, args.resource, args.public_buffer,
                                   args.public_batch)
                    if record["checked_pixels"] != expected_pixels:
                        raise ValueError("checked pixel count changed")
                    if record["target_hash"] != expected_hash:
                        raise ValueError("physical pixel hash changed")
                    if last_sequence is not None and record["sequence"] != last_sequence + 1:
                        raise ValueError("request sequence did not advance by one")
                except ValueError as error:
                    print(f"FAIL round={round_index} shape={shape_index}: {error}", file=sys.stderr)
                    return 1
                last_sequence = record["sequence"]
                records.write(json.dumps({"round": round_index, "shape": shape_index,
                                          **record}) + "\n")
                records.flush()
    meta["end_unix_ns"] = time.time_ns()
    meta["final_sequence"] = last_sequence
    meta["draws_passed"] = args.rounds * len(SHAPES)
    (args.output / "metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    label = ("public-batch" if args.public_batch else "public-buffer" if args.public_buffer
             else "separate-page" if args.resource else "user-vertex")
    print(f"PASS {meta['draws_passed']} physical N100 {label} draws; "
          f"final sequence {last_sequence}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
