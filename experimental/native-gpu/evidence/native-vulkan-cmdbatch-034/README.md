<!-- SPDX-License-Identifier: MIT -->
# Multiple command buffers from one Vulkan allocation

`vkAllocateCommandBuffers` now accepts a nonzero primary command-buffer count
and returns distinct buffers from the same pool. If an allocation fails, it
removes the buffers created by that call and clears their output handles. The
VM command-reset probe checks a two-buffer allocation, distinct ownership,
and freeing one buffer while the other remains in the pool. With the old
library the probe exited 22 at this check; with the new library it passed.

The physical N100 ran the updated triangle probe with `commandBufferCount=2`
in a single allocation. [Batch trace](physical-batch.log) records submissions
50 and 51, each with 777 matching checked pixels and final hash `28616d2d`.
[Dual-pipeline trace](physical-dual.log) records submissions 52 and 53 with
different fragment shader BOs and the expected final hash `1629ed2d`. The
[single-buffer regression](physical-single.log) also passed. Each run used
the isolated `/boot/home/native-vulkan-cmdbatch-034` bundle; no installed
owner, bridge, compiler, or kernel component was changed.

These are synchronous native owner submissions. This checkpoint does not add
secondary command buffers, simultaneous queue use, Vulkan semaphores, or
multiple draws within one render pass.
