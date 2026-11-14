#!/usr/bin/env python3
"""Repeat the installed fixed RCS draws on the physical N100 over SSH."""

import argparse
import json
import pathlib
import re
import subprocess
import sys
import time

EXPECTED_HASHES = (0x28616D2D, 0x814E0C35, 0x77323FD5, 0xD8B0A07D)
CLIENT = "/boot/home/native-rcs-owner-005/build/gpu_render_sample"


def run(*args):
    return subprocess.run(args, text=True, capture_output=True, timeout=20)


def parse(output, public_shader=False):
    def pair(pattern, base=10):
        match = re.search(pattern, output)
        if not match:
            raise ValueError(f"missing field: {pattern}")
        return tuple(int(value, base) for value in match.groups())

    if public_shader:
        status, stage = pair(r"rcs_public_shader ioctl=0 status=(-?\d+) version=6 stage=(\d+)")
        nonce = observed_nonce = None  # The v6 client checks nonce before PASS.
        success = "RCS public shader hardware draw: PASS"
    else:
        status, stage = pair(r"native_render version=1 mode=3 ioctl=0 status=(-?\d+) stage=(\d+)")
        nonce, observed_nonce = pair(r"nonce=([0-9a-fA-F]+)/([0-9a-fA-F]+)", 16)
        success = "RCS fixed-pipeline shader draw: PASS"
    checked, matched = pair(r"checked=(\d+) matched=(\d+)")
    image_hash, = pair(r"target_hash=([0-9a-fA-F]+)", 16)
    sequence, observed = pair(r"sequence=(\d+)/(\d+)")
    if success not in output:
        raise ValueError("client did not confirm draw")
    if status != 0 or stage != 12 or checked != 654 or matched != checked:
        raise ValueError("render status or pixel count changed")
    if sequence != observed or (nonce is not None and nonce != observed_nonce):
        raise ValueError("completion evidence differs from submitted request")
    return {"sequence": sequence, "checked_pixels": checked,
            "target_hash": f"{image_hash:08x}"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rounds", type=int, default=16)
    parser.add_argument("--host", default="renkuos-n100")
    parser.add_argument("--client", default=CLIENT)
    parser.add_argument("--public-shader", action="store_true")
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.rounds <= 1000:
        parser.error("--rounds must be 1..1000")
    args.output.mkdir(parents=True, exist_ok=False)
    meta = {"client": args.client, "host": args.host, "rounds": args.rounds,
            "public_shader": args.public_shader,
            "expected_hashes": [f"{value:08x}" for value in EXPECTED_HASHES],
            "start_unix_ns": time.time_ns()}
    digest = run("ssh", args.host, "sha256sum", args.client)
    if digest.returncode:
        raise SystemExit(digest.stderr or "could not hash installed client")
    meta["client_sha256"] = digest.stdout.split()[0]
    (args.output / "metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    last_sequence = None
    with (args.output / "draws.jsonl").open("w") as records, \
         (args.output / "draws.raw.log").open("w") as raw:
        for round_index in range(args.rounds):
            for variant, expected_hash in enumerate(EXPECTED_HASHES):
                result = run("ssh", args.host, args.client,
                             *(() if args.public_shader else ("draw",)), str(variant))
                raw.write(f"round={round_index} variant={variant} exit={result.returncode}\n")
                raw.write(result.stdout + result.stderr + "\n")
                raw.flush()
                try:
                    if result.returncode:
                        raise ValueError(f"client exited {result.returncode}")
                    record = parse(result.stdout, args.public_shader)
                    if record["target_hash"] != f"{expected_hash:08x}":
                        raise ValueError("pixel hash differs from physical baseline")
                    if last_sequence is not None and record["sequence"] != last_sequence + 1:
                        raise ValueError("request sequence did not advance by one")
                except ValueError as error:
                    print(f"FAIL round={round_index} variant={variant}: {error}", file=sys.stderr)
                    return 1
                last_sequence = record["sequence"]
                records.write(json.dumps({"round": round_index, "variant": variant,
                                          **record}) + "\n")
                records.flush()
    meta["end_unix_ns"] = time.time_ns()
    meta["final_sequence"] = last_sequence
    meta["draws_passed"] = args.rounds * 4
    (args.output / "metadata.json").write_text(json.dumps(meta, indent=2) + "\n")
    print(f"PASS {meta['draws_passed']} physical N100 shader draws; final sequence {last_sequence}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
