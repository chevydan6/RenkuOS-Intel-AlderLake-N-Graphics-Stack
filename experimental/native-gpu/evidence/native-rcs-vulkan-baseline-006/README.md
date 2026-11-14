# Physical N100 RCS regression, 2026-11-14

The `render/rcs_hardware_regression.py` runner executed 16 rounds of the four
existing fixed shader draws on the physical RenkuOS N100 (`8086:46d1`, revision
0). All 64 requests returned success, matched the four established pixel hashes,
checked 654/654 pixels, matched their nonce and completion sequence, and advanced
the sequence without gaps through 2746. A read-only probe passed after the run.

`metadata.json` records the installed client hash and run interval.
`draws.jsonl` has one compact record per request. `draws.raw.log` retains the
full client reports for fault analysis. The SHA-256 of the JSONL evidence is
`845c1413f19233c4b9ae23dcad05fa3ecf2d43bfc1a4a8faee36bc86c67cbcf0`;
the raw log SHA-256 is
`b8d3feb85bd9b69e2e8576f946f80dce431e38c106d7ada0cdc869aba523c2d5`.

The fixed workload is the known-good RCS control for later Vulkan development.
This test does not exercise general user buffers, command buffers, fences, or a
Vulkan application. No installed kernel or graphics binary was changed.
