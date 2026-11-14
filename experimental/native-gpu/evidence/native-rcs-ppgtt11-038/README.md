<!-- SPDX-License-Identifier: MIT -->
# Eleven-page owner mapping verified on N100

The owner now maps eleven private RCS draw pages at `0x400000` through
`0x40a000`, reserving four pages for a future 64×64 target and the last page
for post-sync scratch. The separate legacy vertex resource moved from
`0x408000` to `0x40b000` to avoid overlap. A new compile-time regression
first failed on the old address, then passed after the move. The native
PPGTT helper already supports eleven data pages and binding the vertex page
after them. The 32×32 request and its 4 KiB ioctl output are unchanged.

The isolated RenkuOS VM owner build and all CPU smokes passed
([build log](vm-build.log)). Candidate owner SHA-256:
`2c348d1df190451644c0e80725171c63c98b54b8aa4136e21acabd27c0e3f148`.
The previous installed owner, SHA-256
`16408adc09d8036d03dd9f07287308893da8d64f62fe8c8bd5d60f915876a900`,
was saved at `/boot/home/native-rcs-ppgtt11-038/rollback/intel_memory_owner`.
Only the owner was replaced; the bridge was unchanged. The N100 rebooted and
reported the candidate hash as its installed owner.

[Physical owner results](physical-owner.log): read-only probe passed;
bootstrap retired sequence 1; fixed draw sequence 2 returned hash `28616d2d`
with 654/654 checked pixels; the **relocated separate vertex** draw sequence
3 returned the same hash with 777/777 pixels; combined public vertex and
shader draw sequence 4 also passed. [Vulkan results](physical-vulkan.log):
the compiler-backed version-7 triangle passed at sequence 5 with hash
`28616d2d`, and the rotated version-8 triangle passed at sequence 6 with hash
`77323fd5`. All reported zero GPU fault and normal completion.

These are actual N100 results for the mapping change and legacy rendering.
No 64×64 draw was submitted yet. That still needs a separate 16 KiB request,
full target validation, and Vulkan image support.
