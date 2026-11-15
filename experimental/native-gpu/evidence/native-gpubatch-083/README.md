# N100 native two-triangle GPU batch, checkpoint 083

The Vulkan driver now combines two adjacent `vkCmdDraw` calls into one native
RCS submission when they use one 32x32 direct target, the same pipeline and
vertex buffer, and contiguous three-vertex ranges. The owner uses version 20
to read 96 bytes of validated vertices, append a second vertex-buffer and
`3DPRIMITIVE` packet before the final completion packet, and inspect the
composite image. `RENKU_VK_NATIVE_BATCH=0` selects the prior two-submission
path. Other draw combinations keep their existing paths.

The owner and Vulkan candidates built on the Haiku VM. The owner CPU suite
includes the new packet and two-triangle image fixtures. The old installed
owner rejected version 20 before creating an owner report, as recorded in
`old-owner-rejection.log`. Rollback copies of the previous owner and bridge
are retained under `/boot/home/native-gpubatch-083/rollback` on the N100.

After installing the new owner and bridge and rebooting, the physical N100
completed two triangles with **one version-20 ioctl and one RCS sequence**.
The owner matched 585/585 checked pixels and reported hash `d23b1ba1`.
`batch32.bin` has **zero mismatches across all 1,024 pixels** against the
union of the two independently captured single-triangle images from checkpoint
082, with both triangles rendered in the same red color. The disabled path
used versions 15 and 19 and produced the same final hash. The 32x32 and 64x64
two-submission multidraw probes also passed on the new owner.

`measure-batch.py` alternated 30 batch and 30 disabled runs, checking the
hardware report and output hash after each. The median sum of blocking Vulkan
`submit_us` was **4,931 µs** for one native batch versus **10,197 µs** for
two owner submissions, a **51.6% reduction** for this exact workload. This is
CPU-observed blocking submit latency; it does not isolate GPU execution time.
The raw samples and summary are in `batch-samples.json` and
`batch-summary.json`.

The installed owner and candidate Vulkan driver also passed the prior **222
regression draws**, **24 image-offset draws**, and **16 memory-offset draws**
on the physical N100. Reproduction scripts and per-case logs are retained
here. `physical-hashes.log` identifies the installed owner, bridge, candidate
Vulkan library and probe, and the unchanged compiler binary.

The selected runtime source inventory is estimated at **15,228,611 bytes**
from checkpoint 081's measured 15,222,168 bytes plus changed and new runtime
files. This is a delta estimate, not a rebuilt dependency closure. It omits
SDK/generated inputs, probes, broader kernel and build sources, and unmeasured
Haiku package inputs. Model token count remains unavailable. The 3–5 MB
source target is still unmet.
