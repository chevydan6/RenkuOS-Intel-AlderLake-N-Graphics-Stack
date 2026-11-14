# Validated public RCS command BO on physical N100, 2026-11-14

The native client created two owner-addressed buffers: one with validated
triangle vertices and one with a 4 KiB RCS command batch. Draw version 5
passed both file-local handles to the owner. Before RCS submission, the owner
checked every dword of the batch against the proven draw image, allowing only
the vertex-buffer address resolved from the owned vertex BO. It also checked
the vertex contents, bounds, and session ownership. The owner then set the
ring's batch start to the public command BO and zeroed the private batch and
vertex pages. A passing image therefore requires GPU reads from both public
buffers. No user-selected GPU address or unchecked command reached RCS.

The [native build](native-build.log) passed. [Installed hashes](installed-hashes.txt)
match owner `6d6abe0a6492a7e27f0a2c2d8046895319ab683b7678b521912697f9e0e08099`
and bridge `d592c8bef427668cacb33338d208f42665fc6dda5ce1c49b862eeb989bf2b005`.
The verified version 4 owner and bridge remain as
`/boot/home/native-rcs-resource-012/known-good-v4-owner` and
`known-good-v4-bridge`. Host ASan/UBSan passed the exact-batch validator and
ring-start tests; the complete native build ran their CPU versions.
[Source hashes](source.sha256) identify the change. No RenkuOS base-kernel code
or Linux DRM/GEM interface changed.

After reboot, read-only probe and explicit RCS bootstrap passed. Four
[invalid requests](final-negative-and-probe.log)—a changed packet word,
nonzero trailing bytes, a second-file forged handle, and a stale handle—were
rejected before submission with zero sequence and output. The first valid
public batch completed with 777/777 checked pixels, hash `28616d2d`, and no
GPU fault.

| Physical regression | Result | Final sequence |
| --- | --- | ---: |
| [64 public-batch v5 draws](v5-64/metadata.json) | Four expected images and gap-free completion | 66 |
| [64 public-vertex v4 draws](v4-64/metadata.json) | Existing baseline preserved | 130 |
| [64 fixed v1 draws](v1-64/metadata.json) | Existing baseline preserved | 194 |
| [64 user-vertex v2 draws](v2-64/metadata.json) | Existing baseline preserved | 258 |
| [64 separate-page v3 draws](v3-64/metadata.json) | Existing baseline preserved | 322 |
| [Four final v5 draws](v5-after-controls/metadata.json) | Public batch still passes | 326 |

Each runner saved structured JSONL and raw client output. Resource transfer,
failed-copyout rollback, repeated invalid-batch rejection, and the final probe
also passed. The physical test lock was released; the Govee recovery outlet
was not used.

This proves native userspace command BO execution on the N100 for one exact,
known-safe RCS template. It is not a general ANV command stream: arbitrary
draw state, shader binaries, sparse or dynamic VM binds, Vulkan queue waits
and signals, asynchronous fences, and presentation remain unimplemented.
