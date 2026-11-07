#!/usr/bin/env python3
"""Replay archived probes to a loopback server; never execute returned tool calls."""

import argparse
import json
from pathlib import Path
import urllib.request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--model", help="Override the archived model alias")
    parser.add_argument("--output", type=Path, required=True,
                        help="New directory outside this evidence bundle")
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("port must be between 1 and 65535")
    root = Path(__file__).resolve().parent
    output = args.output.resolve()
    if output == root or root in output.parents:
        parser.error("output must be outside the published bundle")
    output.mkdir(parents=True, exist_ok=False)
    # Do not send local diagnostic requests through an environment HTTP proxy.
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    endpoint = f"http://127.0.0.1:{args.port}/v1/chat/completions"
    for path in sorted((root / "evidence/probes").glob("*.request.json")):
        body = json.loads(path.read_text())
        if args.model:
            body["model"] = args.model
        data = json.dumps(body, indent=2).encode()
        (output / path.name).write_bytes(data + b"\n")
        request = urllib.request.Request(endpoint, data=data,
                                         headers={"Content-Type": "application/json"})
        with opener.open(request, timeout=300) as response:
            result = json.load(response)
        destination = output / path.name.replace(".request.json", ".response.json")
        destination.write_text(json.dumps(result, indent=2) + "\n")
        print(destination.name, flush=True)
    print("Replay saved. Returned tool calls were not executed.")


if __name__ == "__main__":
    main()
