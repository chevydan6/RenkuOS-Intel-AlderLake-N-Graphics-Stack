#!/usr/bin/env python3
"""Repeat the bounded public vertex plus shader draw on the N100."""
import argparse
import json
from pathlib import Path
import re
import subprocess

DEFAULT_CLIENT = "/boot/home/native-rcs-combined-015/build/gpu_render_combined"
LINE = re.compile(r"combined ioctl=0 status=0 version=7 stage=12 "
                  r"sequence=(\d+)/(\d+) pixels=(\d+)/(\d+) "
                  r"hash=([0-9a-f]{8}) fault=([0-9a-f]{8})")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="renkuos-n100")
    parser.add_argument("--client", default=DEFAULT_CLIENT)
    parser.add_argument("--draws", type=int, default=64)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.draws <= 1000:
        parser.error("--draws must be 1..1000")
    args.output.mkdir(parents=True, exist_ok=False)
    digest = subprocess.run(["ssh", args.host, "sha256sum", args.client],
                            text=True, capture_output=True, timeout=20, check=True)
    client_hash = digest.stdout.split()[0]
    previous = None
    with (args.output / "draws.jsonl").open("w") as records, \
         (args.output / "draws.raw.log").open("w") as raw:
        for index in range(args.draws):
            result = subprocess.run(["ssh", args.host, args.client],
                                    text=True, capture_output=True, timeout=20)
            raw.write(f"draw={index} exit={result.returncode}\n"
                      + result.stdout + result.stderr + "\n")
            raw.flush()
            match = LINE.search(result.stdout)
            if result.returncode or not match or \
               "RCS combined public vertex/shader draw: PASS" not in result.stdout:
                raise SystemExit(f"draw {index} failed: client exit={result.returncode}")
            sequence, observed, matched, checked = map(int, match.groups()[:4])
            image_hash, fault = match.groups()[4:]
            if sequence != observed or (previous is not None and sequence != previous + 1) \
               or matched != 777 or checked != 777 or image_hash != "28616d2d" \
               or fault != "00000000":
                raise SystemExit(f"draw {index} returned unexpected completion or pixels")
            records.write(json.dumps({"draw": index, "sequence": sequence,
                                      "matched_pixels": matched,
                                      "target_hash": image_hash}) + "\n")
            records.flush()
            previous = sequence
    (args.output / "metadata.json").write_text(json.dumps({
        "host": args.host, "client": args.client, "client_sha256": client_hash,
        "draws_passed": args.draws, "final_sequence": previous}, indent=2) + "\n")
    print(f"PASS {args.draws} combined N100 draws; final sequence {previous}")

if __name__ == "__main__":
    main()
