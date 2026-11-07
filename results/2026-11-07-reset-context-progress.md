# N100 reset and context construction — publication checkpoint

2026-11-07. Target8086:46d1/revision00, pinned RenkuOS source
`4799bb5a188c4bb6bbef967287640cc21a061a0d`, kernel hrev60072+518.

**One physical BCS reset succeeded. No GPU command submission, accelerated
rendering, OpenGL or Vulkan operation has been demonstrated.** Native HDMI
1920×1080 remains the validated display milestone, separate from execution.

## Published implementation and observations

| Milestone | Result | Evidence |
| --- | --- | --- |
| A29 inventory | Native build and one physical fixed-register sample passed; tail/head/control0, mode0x200, message-idle0x10; postchecks passed. | [Physical inventory](../tests/evidence/physical-bcs-inventory-001/) |
| A30–A32 domain planning/handshake | Fixed fuse0x000e00fe selected GT/render/VDBOX0/VEBOX0. Physical four-domain handshake and release passed. | [Fuse](../tests/evidence/physical-media-fuse-001/), [group](../tests/evidence/physical-forcewake-group-001/) |
| A33 reset preparation | Failed Unsafe after3 preparation writes,11 reads and0 reset rounds. No GDRST store occurred. Four wake requests and pin retained; exact A32 rollback/reboot/postchecks passed. Root cause remains unknown. | [Failure/recovery](../tests/evidence/physical-bcs-reset-001/) |
| A34 instrumented reset | Bootstrap479us; group2383us; reset1150us,15 reads,6 writes,2 reset rounds. All4 domains Released.49 safety checks/196 ACK reads passed. ACK/PCI/BCS inventory and10 paired-GART rounds passed; preserved hashes and responsive desktop observed. | [Physical success](../tests/evidence/physical-bcs-reset-002/), [host/native](../tests/evidence/reset-safety-host-vm-001/) |
| A35 descriptor/status |2357 host ASan/UBSan and native VM cases passed. Checked Gen12 BCS descriptor and single-request event ledger; hardware copy class3, tags1..2046. | [Software evidence](../tests/evidence/context-status-host-vm-001/) |
| A36 CSB reader/HWSP |135 host/native cases passed. Fixed memory offsets, poison-aware ordered reads, deadlines/iteration caps, final tail checks and retained failures. | [Software evidence](../tests/evidence/csb-reader-host-vm-001/) |
| A37 initial context image |715 host/native cases passed:681 full16KiB comparisons against extracted pinned Linux routines plus34 rejection/guard cases. Source comparison caught and corrected saved STOP_RING maskbit8 before deployment. | [Software evidence](../tests/evidence/context-image-host-vm-001/) |

Patches0093–0096 add fuse observation, single-use group handshake, reset-only
observation and retained reset-safety diagnostics. v0–v7 layouts remain unchanged
by0096; v8 reads retained safety evidence without hardware access. Native reset
bridge/client protocol2 includes that snapshot. Failed ownership is retained,
not cleared by guessing. v6/v7 consume the same one-shot group admission.

A34 remains the latest installed GPU-owner artifact from the recorded trial.
The reset transaction was consumed for that boot; this publication does not
rerun it or deploy the newer offline helpers. A32 recovery artifacts were retained.
One successful trial does not establish repeated reset reliability or explain A33.

## Unfinished and unvalidated work

* A35–A37 are standalone CPU helpers, not production owner integration. Their
  successful event/image checks cannot authorize DMA release or ELSQ publication.
* Integrate distinct context/ring/engine-HWSP/result/PPGTT backing with durable
  ownership, checked CPU/DMA/GGTT address records and failure retention.
* Establish persistent DMA/IOMMU and power-transition contracts, CPU/GPU cache
  visibility, PAT/MOCS programming and TLB invalidation.
* Complete stepping-specific engine initialization/error service, fresh-tag and
  hardware CSB-reset protocol, and completion/retirement/recovery integration.
* A modulo CSB cursor cannot detect an unseen full revolution by itself. The
  isolated single-request protocol must exclude overrun and unsafe tag reuse.
* First execution remains a kernel-built direct-ring nonce/complement store
  requiring ordered completion, matching context switch-away, guards and engine
  inactivity. A nonce or CSB event alone is insufficient. Vulkan follows verified
  command execution and a substantially larger driver/API implementation.

Designs: [reset](../research/n100-submission/BCS-RESET-DESIGN.md),
[status](../research/n100-submission/CONTEXT-STATUS-DESIGN.md),
[CSB reader](../research/n100-submission/CSB-READER-DESIGN.md),
[context image](../research/n100-submission/CONTEXT-IMAGE-DESIGN.md),
[execution plan](../GPU_FIRST_EXECUTION_PLAN.md).

## Independent helper reviews and evidence scope

Qwen's unchanged host artifacts were independently checked: all9 hashes matched,
source copies matched current files, sanitizer linkage and archived32/2150/69
passing results were verified. The original report overstated evidence capture;
the independent review supplies run/exit records. [Review](../coordination/ASTRA_QWEN_OFFLINE_REVIEW.md).

Qwen's native guest PPGTT programs passed2150/69 cases, originals and independent
rebuilds; both rebuilt binaries matched originals byte-for-byte. [Review](../coordination/ASTRA_QWEN_VM_REVIEW.md).
These are host/native userspace software tests, not kernel or GPU execution.
The local QEMU guest has no N100 passthrough. Its matching kernel/SDK supports
native builds and fixtures but cannot establish physical register behavior.

## Reproduction and publication checks

All31 staged-tree preparation/build/test commands passed; see the
[publication check record](../tests/evidence/PUBLICATION-CHECKS-20261107.md).

The GitHub workflow reconstructs all96 patches from the pinned RenkuOS source,
checks the payload, exercises the existing display/GART regressions, and runs
the actual-owner and new bounded reset/context host fixtures. Context-image
reference code is downloaded only from the pinned Linux commit and SHA-256
verified before compilation. Example:

```sh
python3 tools/prepare.py --output build/review
python3 tools/check-source.py build/review
python3 tests/owner-observation-host.py --source build/review/source --output build/owner-review
python3 tools/fetch-context-image-references.py --output build/context-references
python3 tests/context-image-host.py --references build/context-references --output build/context-review
```

Historical native/physical logs and artifact identities are preserved as selected
evidence. Build trees, downloaded source caches, binaries, credentials, access
configuration, lock/session metadata and full host process dumps remain local.
`published.sha256` files identify each selected evidence subset where applicable;
historical full collection manifests may remain local with omitted raw captures.
Useful cumulative coordination records are published as chronological history;
earlier pending states are superseded by the results above. Incorrect superseded
audit drafts and the independent touchpad worktree are not part of this checkpoint.
