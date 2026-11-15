# Vulkan LOAD across queue submissions, checkpoint 093

The `loadsubmit` probe records one triangle in a `DONT_CARE` render pass and
a complementary triangle in a `LOAD` render pass, using separate command
buffers and separate `vkQueueSubmit` calls. It waits for the first fence,
resets that fence, submits the second command, and waits again before mapping
and invalidating the image. This exercises preservation across the queue-call
boundary, using existing owner requests 15 and 19; the kernel and Vulkan
library are unchanged.

`run-hardware.py` runs three repetitions at each of 32×32 and 64×64. It checks
the trace for versions 15 then 19, one GPU draw per submission, increasing
completed sequences, the same image BO, no ioctl or GPU fault, and complete
pixel verification. It saves the raw capture from every repetition and checks
its full SHA-256 against the checkpoint-091 same-pass `multidraw` reference.
The expected final image hashes are `225b1ba1` and `bb1f873b`.

The probe was compiled on `renkuos-vm` with the Mesa 26.3.0 Vulkan headers,
then run on the physical N100 with `libvulkan.so` from checkpoint 091 and
the Intel compiler from checkpoint 092. No kernel module was changed.
The tested probe SHA-256 is `f9aed0c58d3e611381575455198ef94ee779152c0719f4d37ad6d6601c1deef8`;
the loaded Vulkan and compiler library SHA-256 values are
`07c02bdec6ce5274e602847abb8afde8ccc4e8569ed71874ef21a8872b1b5be4`
and `7b195b9f71116ae92324d06d77b8e63bfe0250bbc7c9e4e385cf21615c70f717`.
The existing `loadpass` probe also passed at both sizes with this executable.
Only probe and evidence files changed, so the selected runtime-source inventory
remains **14,810,595 unique bytes**; the model tokenizer count remains unknown.
