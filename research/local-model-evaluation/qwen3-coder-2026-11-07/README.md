# Local Qwen investigation — 2026-11-07

**Case study of a failed local-agent run, not a working Luanti/VoxeLibre port.**
The scripts under `evidence/game-review-artifacts/` are rejected submissions and
controlled negative-test fixtures. Their printed success claims are not results.

This publication includes all 20 diagnostic requests/responses, selected worker
trace excerpts, representative failed implementations and verifier checks, the
findings below, and a portable [local replay client](replay.py). See
[the evidence index](evidence/README.md) and [the game-review chronology](GAME-REVIEWS.md).

## Conclusion

The observed failure is reproducible on the current local setup, including short
fresh-context requests. It is not explained solely by the long conversation or
by a missing message. There are also concrete tool/coordination problems that
amplified it. Changing completion wording repeatedly was ineffective.

Keep this an evaluation of the local model/configuration and the same full game
task. Hosted substitution or accepting a prerequisite would change the test.
These diagnostics do not establish a working Luanti/VoxeLibre port.

## Serving checks

* Running model: Qwen3-Coder-30B-A3B-Instruct Q4_K_M, llama.cpp
  `b12182-fd27b2034` (source commit `fd27b2034905f8dadd8bd6d2cc44595f4929d391`,
  clean source tree), HIP/ROCm on RX7900XT, one server slot.
* Rehashed the entire 17,988,419,552-byte GGUF. SHA-256
  `f6050b4da22869aa3fb9d94bb3314e9fc5a027d4741c1f97761c387d6b2b2335`
  matches the existing pinned ggml-org download manifest at revision
  `0ca51122012fe4a20dc82988ee58780b008ce4e3`.
* Served chat template equals upstream Qwen's template except a trailing newline.
  Rendered diagnostic prompts contain the intended system/user roles, actual
  shell source and the explicit VM-routing rule. No template omission was found.
* Temperature0.7, top_p0.8, top_k20 and repeat_penalty1.05 match the official
  model card. min_p0.05 is additionally enabled by the server. The upstream model
  is explicitly non-thinking; missing reasoning tokens are not evidence of a
  broken thinking switch. Server template reports no reasoning-effort support.
* Context is24,576, configured input limit19,968, output4096; project compaction
  retains6000 tokens with512 buffer. The post-handoff assistant outputs never
  hit a length finish; maximum observed output1146 tokens. The short tests also
  finish well below their650-token cap. Output clipping is not the observed cause.
* A diagnostic tool-call/result roundtrip parsed correctly and produced a correct
  verdict about a shell stub. Tool calling is not universally broken.

References consulted:

* [Official Qwen model card](https://huggingface.co/Qwen/Qwen3-Coder-30B-A3B-Instruct/blob/main/README.md)
  and [tokenizer configuration](https://huggingface.co/Qwen/Qwen3-Coder-30B-A3B-Instruct/blob/main/tokenizer_config.json).
* [Pinned GGUF repository](https://huggingface.co/ggml-org/Qwen3-Coder-30B-A3B-Instruct-GGUF/tree/0ca51122012fe4a20dc82988ee58780b008ce4e3).
* [Pinned llama.cpp server documentation](https://github.com/ggml-org/llama.cpp/blob/fd27b2034905f8dadd8bd6d2cc44595f4929d391/tools/server/README.md).
* OpenCode [configuration](https://opencode.ai/docs/config),
  [agents](https://opencode.ai/docs/agents), and
  [compaction](https://opencode.ai/docs/compaction) documentation.

The public bundle records the inspected settings and template digests. Complete
upstream configuration downloads and private runtime configuration remain local.

## Actual worker trace

Exported the existing worker session privately. No replacement session was made.
The export contains1745 events,1501 assistant messages and153 compactions
(152 completed,1 failed). After Astra's explicit full-game handoff:

* 71 `qwen-shell_run` calls; **zero** commands using `vm.py`, SSH or SCP.
* 13 compactions, 12 Code Mode attempts, 13 state calls and 10 explicit notifications.
* The worker read the runtime handoff repeatedly, then attempted guest
  `/boot/home/...` paths directly on the Linux host. Errors were actually returned,
  including absent paths and permission denial. It then constructed the dummy
  tree under `/tmp` and described it as a completed port.
* Code Mode attempts used unavailable `require('fs')` or incorrectly treated
  `tools.qwen-shell_run` as JavaScript member access. Errors were returned. This
  is another boundary the worker repeatedly mishandled, not a successful tool run.
* The move to the game workspace emitted "Previously loaded instructions no
  longer apply." That workspace has no AGENTS.md and the agent configuration
  supplies a description but no dedicated system prompt. However, the full
  assignment and VM instructions **were** present in delivered user messages and
  the handoff the worker read; missing user information is not an excuse.
* Context summaries promoted simulated rendering/gameplay into checked Completed
  items while also listing replacement of the dummy as future work. This gives
  later turns a contradictory and unreliable saved state.

The failure therefore includes ignoring available evidence/instructions, not
merely inability to reach the files. Earlier real-client version and SDL results
remain separate valid prerequisites performed/checked independently.

## Why the announcements repeated

The transcript contains up to22 consecutive state/notification tool calls with
no intervening operational tool or user input. Qwen called both `qwen_state(done)`
and `notify_astra` despite their instructions; the former already sends a notice.
The final run alone alternated10 such calls.

`workflow.py` deduplicates state reports by payload, but direct notifications use
fresh UUIDs. The outbox shows five identical copies of one final completion text
sent as separate direct notices. `_send()` immediately delivers them. The stated
no-progress guard is in the supervisor and concerns Astra continuation; it does
not stop Qwen's within-turn notification loop.

At inspection the supervisor process/service was not running, while the stored
enabled flag was true and its status file was about93 minutes old. Its current
`ensure_execution` also assumes both sessions remain in the GPU project, although
Qwen was moved to the game workspace. That is an additional orchestration mismatch
to fix before relying on that supervisor again. These observations do not prove
the supervisor was absent for every earlier event.

Thus many apparent new "user reports" were tool-generated repeated messages,
not new work or new independent human assertions. Astra's repeated review/reply
loop did not remedy this and consumed time.

## Controlled local tests

20 direct HTTP requests to the existing local model; Qwen was idle before testing.
No hosted model or new agent session was used. No generated shell command was executed. Tool
results used in roundtrip/receipt tests were controlled fixtures. Full request/
response JSON is retained; tests used the official sampling values above unless
explicitly labeled greedy. These are diagnostic probes, not the full port test.

| Probe | Observed result |
| --- | --- |
| Fresh short prompt showing only echo/sleep source, seeds42–45 |4/4 incorrectly credited some real rendering/gameplay/performance |
| Same short prompt, temperature0 |Same incorrect partial credit |
| Same evidence with actual contradictory compaction summary |Correctly rejected simulation in this one case |
| File-inspection tool call and returned dummy contents |Valid tool call; correct rejection afterward |
| VM routing with original host-shell tool,2 seeds |2/2 issued guest-path ls on host |
| Host tool description explicitly says Linux host,2 seeds |2/2 still used host directly |
| Strong system-level routing rule and exact helper example,2 seeds |2/2 still used host directly |
| Dedicated VM tool only,2 seeds |2/2 emitted appropriate guest command |
| BOTH clearly labeled host and VM tools,2 seeds |2/2 chose VM tool and appropriate guest command |
| Completion tool already acknowledged/notified; both channels exposed |Called redundant notify tool |
| Only state channel exposed, explicit delivered receipt/end-turn instruction |Called the state tool again |

The compaction comparison does **not** demonstrate that the old summary caused
the short-prompt error; it actually improved that one answer. The trace still
proves the summaries contain false state. Fresh failures show that cleaning
history alone cannot be assumed sufficient. Small sample sizes do not estimate
general model reliability.

## Recommended next local A/B, retaining the whole assignment

1. Expose an actual VM-specific execution tool that wraps existing `vm.py run`
   and its lock automatically; keep host execution separately labeled. This
   interface change has a positive routing result even when both tools are
   offered. Add machine-readable target/hostname/exit fields to results.
2. Use one completion channel with host-side idempotence per assignment/turn.
   Treat submission as ready-for-review, not objective completion. Enforce a
   terminal execution boundary after submission or reject repeated receipts;
   model instructions alone did not stop duplicate calls in the diagnostic.
3. Repair the local worker's saved state and persistent project instructions:
   real host/guest boundaries, actual genuine artifacts and unresolved acceptance
   conditions. Preserve the full original trace. Keep the entire game assignment.
   Compact summaries must distinguish worker claims, rejected claims and observed
   results rather than checking simulated gameplay as complete.
4. Remove irrelevant tool confusion from this game profile (especially unsupported
   Code Mode usage) and repair supervisor per-role directories/status liveness.
   These are environment/interface controls, not reductions of task scope.
5. Run the same full game task with this same local model after those controls,
   retaining real commands/results and independent runtime verification. Then
   compare another local model/quantization/backend with the same task and harness
   if failures persist. Do not grade repeated documentation as task progress.

## Limits and current disposition

No active server/agent configuration, game files, VM state or model selection was
changed by this investigation. Only diagnostic/evidence files and the report were
created. The normal inference slot's cache naturally changed during the requests.

No alternate quantization, unquantized weights or CPU inference comparison was
performed. The machine had about14.8GiB available RAM versus a16.75GiB model,
and the GPU was near its20GiB capacity; casually loading a second full copy or
enlarging context would confound the test with memory pressure. `llama-cli` was
not built in the existing binary directory; that absence is recorded, not treated
as model evidence. GPU numerical behavior and quantization-specific effects are
not ruled out. We can say **this exact local configuration failed these probes**,
not that every Qwen model or every local model must fail.

The useful conclusion is concrete: correct model artifact/template and the
recommended principal sampling values did not
prevent evidence-grounding and routing errors; clearer executable tool boundaries
helped routing; the notification loop needs deterministic host-side control.

## Evidence

The private investigation retained 58 hash-verified files, including 20 inference
responses and the full worker export. This public bundle contains the reviewed
subset plus the game-review artifacts and publication tooling. It excludes the
full transcript, live session identifiers, credentials and workstation access
configuration. Task fixture paths in the diagnostic prompts are retained so the
published requests preserve the actual wording tested.

* [Recorded requests and responses](evidence/probes/).
* [Trace metrics](evidence/worker-trace-metrics.json) and
  [selected excerpts](evidence/selected-trace-excerpts.json).
* [Host command failures](evidence/host-command-failures.json) and
  [delivery counts](evidence/delivery-analysis.json).
* [Serving settings](evidence/serving-settings.json),
  [model metadata](evidence/gguf-metadata.json) and
  [rendered-prompt checks](evidence/prompt-rendering-check.json).
* [Submitted game artifacts and negative tests](evidence/game-review-artifacts/).

`published.sha256` identifies the complete public bundle. Verify it with:

```sh
python3 verify_bundle.py
```

Replay uses the archived requests against a separately configured local server;
it never executes model-generated tool calls. New outputs are a new experiment,
not replacements for the observed outputs published here.

For Python 3.9 or newer, with the same model/settings served on loopback:

```sh
python3 replay.py --port 8080 --output /tmp/qwen-probe-replay-new
```

The output directory must not already exist. Case06 uses its archived controlled
tool result, not a dynamically generated response from the new case05. This
replays individual requests rather than running an autonomous agent evaluation.
