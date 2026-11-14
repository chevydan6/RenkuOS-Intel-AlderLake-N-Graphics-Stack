# Native Vulkan ordered batch submission on N100

`vkQueueSubmit` now accepts multiple submit records and command buffers when
they contain no semaphores. It validates every record and command before
submitting any work, then executes each draw synchronously and signals the
optional fence only after the last successful draw. The one-submit path and
the existing empty-submit fence behavior remain available.

The test was written first in `triangle_probe.cpp`. Against the previous
library it failed at the expected two-submit call with code 55 and no draw.
The new library SHA-256 is
`5e0b1ed16a1f5c34d696cf42e39e2aa6e20f47a57784ddb4182dba120216cac1`;
the test application SHA-256 is
`d1dc7ce3e6f23937194babaadd26b34781714e15db232ad693a333d8559e2a73`.
Both were staged under `/boot/home/native-vulkan-batch-025`; the installed
kernel owner and original Vulkan checkpoint were not replaced.

With `RENKU_VK_TRACE=1`, one two-submit call produced owner sequences 232 and
233, each with checked/matched pixels 777/777, pixel hash `6f2bc92d`, no
reported fault, and successful image-BO writes. The fence was unsignaled
before the call and signaled after both draws. The raw record is [trace.log](trace.log)
(SHA-256 `98c8442cf4b399458b7ddc7a8423aaee5b84223281e3ca68206511b451d77a7b`).

The [matrix](matrix.log) ran all four triangle shapes through two-submit and
single-submit paths. Both returned the established exact hashes:

| Shape | 0 | 1 | 2 | 3 |
|---|---|---|---|---|
| Hash | `6f2bc92d` | `a4e13743` | `ce445d05` | `fffe3dc5` |

The matrix log SHA-256 is
`c800d3aa00dc7497ff8b0746ce83766d0c4dc65a127b09e4643f4fb849f9cce2`.
The VM compiler, Vulkan API, fence, and command-reset probes passed after the
library change. This remains a fixed 32×32 pipeline with synchronous owner
draws and CPU image publication; semaphores and general Vulkan submissions
are not yet implemented.
