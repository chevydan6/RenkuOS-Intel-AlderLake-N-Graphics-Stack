# Evidence scope

## Diagnostic inference

`probes/` contains the 20 original HTTP request bodies and 20 parsed responses
from the local llama.cpp server. Authentication headers are not included. The
two rendered prompts show that the system instructions and relevant user content
were present in the applied chat template.

The tool schemas and results in these probes are diagnostic fixtures. Generated
tool calls were recorded, never executed. In the original roundtrip, case05's
actual tool call was supplied a controlled file-inspection result for case06.
Every archived request is also self-contained for independent replay.

Paths such as `/boot/home/...`, `/home/dan/...` and `/tmp/luanti-final` are the task
strings that the model saw. They do not grant access to a live machine. A replay
client must not execute any returned commands or use the completion tools to send
real notifications.

## Worker observations

`selected-trace-excerpts.json` contains relevant message/tool contents with live
message/session identifiers and top-level message timestamps removed. The excerpts cover host/VM
confusion, repeated completion reporting, false compaction state, and the
instruction change on moving workspaces. Metrics were computed from the original
private export. This is a selected disclosure, not the entire transcript.

`host-command-failures.json` preserves command/output pairs from the full-task
phase; `delivery-analysis.json` records outbox counts and repeated notice text.
These establish what was attempted and reported, not a working port.

## Rejected game submissions and verifier tests

`game-review-artifacts/` intentionally preserves defective scripts and false
completion claims as evidence. **None is a supported launcher or passing game
test.** Submitted Markdown files are named `*.submitted.md` to distinguish them
from the investigator's findings.

The review010 and review011 negative tests were performed on isolated host
fixtures, with only hardcoded path prefixes changed in the submitted verifier.
Their stubs deliberately exit42. Result JSON and logs show whether the verifier
falsely reported success; those successes do not describe actual game execution.

Review014's VM inspection used the existing transaction lock and only inspected
paths. Its exit0 denotes a completed inspection, while its log explicitly reports
the claimed runtime paths as missing.
