# Native Vulkan binary semaphore submissions (064)

The native Vulkan device now creates binary semaphores and accepts waits and
signals in `vkQueueSubmit` on its one synchronous graphics queue. It validates
all command buffers and simulates semaphore state across every batch before
the first GPU draw. A missing or unsignaled wait, duplicate signal, foreign
semaphore, or missing wait-stage mask is rejected without partial submission.
Waits are consumed before their batch and signals become visible after its
draws complete. Fence signaling remains last. This supports the
[Vulkan binary semaphore signal/wait ordering](https://docs.vulkan.org/spec/latest/chapters/synchronization.html)
for this queue; it does not add asynchronous execution, multiple queues,
timeline semaphores, or presentation semaphores.

The [VM build](vulkan-vm-build.log) passed the new `semaphore_probe` plus
compiler, API, fence, and command-reset probes. The probe tests signal-then-wait
within one call, consumed waits, duplicate-signal preflight, cross-device
rejection, and creation validation. The [isolated N100 bundle](native-bundle-build.log)
uses the unchanged 062 Intel compiler and the installed 063 owner. No owner,
bridge, base-kernel, or global Vulkan library replacement was needed.

The physical N100 passed **14 semaphore cases and 28 GPU draws** in
[semaphore-matrix.log](semaphore-matrix.log): 32×32 and 64×64 triangle shapes,
RGB, red and green gradients, and both direct and fallback image paths.
Each case first attempts a two-batch submission with an unsignaled wait,
which must return before any draw, then submits a valid signal/wait pair.
Exactly two successful submission events appear per case; each completed
without an ioctl error or GPU fault and matched the expected pixels. The
previous rendering matrix, supplemental cases, fallback cases, and green
gradients add **178 regression draws** with this same Vulkan library. Run
`python3 verify.py` to check all **206 draws across 68 cases** in the saved
traces. [Physical hashes](physical-hashes.log) identify the exact owner,
bridge, compiler, Vulkan library, and probe binaries.

The [Vulkan input manifest](vulkan-manifest.json) and [combined stack
manifest](stack-manifest.json) measure **12,633,124 unique bytes of selected
canonical runtime source**, 3,709 bytes above checkpoint 063. Build-only
generators, SDK headers, some external implementation source, and model tokens
remain outside that figure. The 3–5 MB experimental target and broad Vulkan
compatibility remain open.
