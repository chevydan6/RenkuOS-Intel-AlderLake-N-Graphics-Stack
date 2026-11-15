# N100 64x64 native GPU batch, checkpoint 084

Version 20 of the native owner request now accepts a 64x64 direct target as
well as 32x32. It uses the existing 96-byte contiguous vertex resource and
two-primitive RCS packet extension. The owner validates both triangles,
rejects nonconstant fragment programs for this request, and inspects the
four-page final image. Vulkan automatically selects the batch for adjacent
draws with the same pipeline and vertex buffer; `RENKU_VK_NATIVE_BATCH=0`
selects the two-submission path.

The owner and Vulkan candidates built on the Haiku VM. The CPU suite includes
the 64x64 packet template and an image mask derived from independent N100
single-draw captures. The old owner rejected the large version-20 request
before execution (`old-owner-rejection.log`). Rollback copies of the previous
owner and bridge are saved on the physical machine in
`/boot/home/native-gpubatch64-084/rollback`.

After installing the new owner and bridge and rebooting, the physical N100
completed two 64x64 triangles in **one version-20 ioctl and one RCS sequence**.
The owner matched 3,016/3,016 checked pixels and reported image hash
`bb1f873b`. `verify-image.py` compares the raw batch output against the union
of two independently captured single-triangle images: **zero mismatches in
all 4,096 pixels**. The 32x32 batch likewise had zero mismatches in 1,024
pixels. Version-15/19 fallback and the older two-submission multidraw probes
also passed at both sizes.

`measure-batch.py` interleaved 20 batch and 20 fallback runs per size. Every
run checked the owner reports and final image hash. Median blocking Vulkan
`submit_us` was:

| Target | One GPU batch | Two publications | Reduction |
| --- | ---: | ---: | ---: |
| 32x32 | 4,869 µs | 10,072.5 µs | 51.7% |
| 64x64 | 4,893.5 µs | 10,218 µs | 52.1% |

These are CPU-observed blocking submission times for this exact workload, not
isolated GPU execution times. `batch-samples.json` and `batch-summary.json`
retain the measurements.

The installed owner and candidate Vulkan library passed the prior **222
regression draws**, **24 image-offset draws**, and **16 memory-offset draws**
on the physical N100. The reproduction scripts and per-case logs are included.
`physical-hashes.log` identifies the tested binaries; the compiler remains
the previously validated Mesa 26.3.0 candidate.

Selected runtime source is estimated at **15,230,106 bytes** by applying the
changed runtime file sizes to checkpoint 081's 15,222,168-byte inventory. This
is a delta estimate, not a rebuilt full dependency closure. SDK/generated
inputs, probes, broader kernel/build sources, and some Haiku package inputs
remain outside that inventory. Model tokenizer measurement remains unavailable;
the 3–5 MB source objective is unmet.
