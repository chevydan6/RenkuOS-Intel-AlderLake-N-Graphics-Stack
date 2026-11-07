# Selected A29–A37 publication evidence

This checkpoint publishes reviewed text results, test inputs/hashes, native build
logs, artifact checks and bounded physical diagnostic/postcheck output. Each new
evidence directory has a `published.sha256` for its committed subset.

Full workstation process/image listings, transient lock leases, reboot polling
and access metadata remain in local collections. They are not needed to replay
the software tests and may contain unrelated workstation details. Historical
collection `manifest.sha256` files remain local when they include omitted files;
`published.sha256` does not claim to describe those full collections.

The per-trial READMEs retain historical provenance and local archive identities.
References there to a raw system capture or lease record identify the local
collection; they are not promises that every capture is in this repository.
Installed binary hashes, sample results, postchecks and native/compiler identity
needed for the published milestones are included.

A33 is a preserved **failed** experiment, with recovery evidence. A34 is one
successful reset-only experiment. A35–A37 and the Qwen reviews are software
evidence. No logs establish GPU workload execution, safe post-submission DMA
retirement or working Vulkan. Current scope is summarized in
[`results/2026-11-07-reset-context-progress.md`](../../results/2026-11-07-reset-context-progress.md).
