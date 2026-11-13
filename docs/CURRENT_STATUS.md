# Intel N100 GPU — current evidence status

Audit date: 2026-11-13. This document is a cross-branch index, not a claim of fresh testing.

| Capability | Status | Evidence / limitations |
| --- | --- | --- |
| N100 PCI identity | Physically observed | `README.md`, `results/2026-11-07-gpu-progress.md` |
| 1920x1080 HDMI desktop | Physically observed | Known-working display build 057; `results/2026-11-06-hdmi-integration.md` |
| Memory/GART | Partial; host/native and selected physical tests | Not proof of coherent GPU DMA or execution |
| GT forcewake | Single physical handshake reported | `tests/evidence/physical-forcewake-001/` |
| BCS reset | One bounded physical trial reported | `tests/evidence/physical-bcs-reset-002/`; not command submission |
| Context/ring helpers | CPU-side host/native checks | Not integrated into live GPU submission |
| LinuxKPI experiment | Native VM and physical CPU kernel-service tests reported | `experiment/i915-linuxkpi:PORT_PROGRESS.md`; incomplete compatibility and submission |
| GPU nonce store | NOT demonstrated | `GPU_FIRST_EXECUTION_PLAN.md` specifies target |
| GPU 3D acceleration | NOT demonstrated | No completed command execution |
| OpenGL / Vulkan | NOT demonstrated | Downstream milestone |

## Single next milestone
Prove an actual GPU-executed nonce/complement store into GPU-visible memory, with independently verified results, completion/retirement evidence and a disabled-submission negative control. This requires real memory mappings, execution context, engine admission, request lifecycle and CPU/GPU visibility; CPU packet generation alone is insufficient.

## Source of truth and updates
Every future claim must identify branch + commit, test environment (host / native VM / physical N100), exact test artifact and result. Keep historical claims in their original reports; update this index only after review. An agent cycle marked 'worked' is not a hardware milestone.
