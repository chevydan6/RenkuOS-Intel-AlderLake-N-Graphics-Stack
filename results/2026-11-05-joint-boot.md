# Joint GPU/Wi-Fi boot preparation

Candidate `gpu-physical-005` passed preparation and two consecutive cold boots
in QEMU USB/UEFI. Each guest requested power-off and exited zero. The final EFI
archive passed GPU metadata consistency checks and contained a fresh Wi-Fi
subtree with hook status zero and the expected absent-physical-radio report.
This validates orchestration, not Intel GPU acceleration or physical Wi-Fi.

- Image size: 690,814,976 bytes.
- Image SHA-256:
  `a41058166c10e7b67bb6efec1fcd215d4ab15236eae71568abc4377bfdc0f672`
- Preserved source SHA-256:
  `0a8b87be6986acb3bf39c9a4f21431b1e75ebf56eb5a41fd0e51c479877a62a0`
- Coordinator: `astra-astra-gpu-3f9cc92a4aeb.service`, exit zero,
  2 minutes 36.350 seconds, peak memory 922.5 MiB.
- Private archive: Fedora `testing/gpu-physical-005/GPU-VM.TGZ`.

Astra #1 supplied frozen scripts at Wi-Fi commit `ee5be69`. Their handoff
hashes are checked before image preparation. The wrapper uses the baseline's
existing private association helper/settings and runs synchronously before GPU
archive/export/shutdown, bounded to 140 seconds plus termination grace. Failure
is recorded and does not prevent GPU export or recovery.

Candidate 004 passed one cold boot. Review then found that reusing its output
directory would make the Wi-Fi wrapper refuse a subsequent run. Startup now
moves previous results into a unique preserved directory. Candidate 005's two
cold boots exercise this path and validate the final fresh archive.

An initial candidate 005 preparation attempt stopped at its disk-space guard
before creating an image. The failed candidate 002 raw copy had been reclaimed
after verifying clean guest exits, but free space was not yet visible to the
guard. A subsequent attempt passed with the original 2 GiB reserve intact.
Source, failure logs and successful earlier candidates were preserved.

NanoKVM staging is a separate operation. Its initial local seed copy exceeded
120 seconds; inspection confirmed a complete-sized inactive file and no remaining
copy process. Resuming the GPU-owned staging file is permitted only after its
writer stops and after checking it is not the active LUN. A complete hash match
is required before use. No physical boot result is claimed here.
