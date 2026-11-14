# N100 owner version 8: validated vertex instruction page

The owner and native bridge now accept render request version 8. Input fields
`variant` and `reserved` remain distinct file-local vertex and fragment BO
handles. The first four input bytes of `output` carry a third, distinct
file-local vertex-shader BO handle. The owner resolves all three in the same
resource session, validates the complete vertex-shader page against the
approved 192-byte depth-quarter program with zero padding, and copies its
validated bytes into the private 4 KiB VS instruction page before GPU
submission. The input array is overwritten with output pixels on return.
Version 7 remains unchanged.

The version-8 test application was built first. Against the previous owner it
failed with no GPU sequence or stage (`red.log`). It also checks that a reused
handle and a one-bit-corrupted vertex instruction page are rejected before a
GPU sequence. The VM built the candidate owner and bridge, and all owner CPU
smokes passed (`vm-build.log`). A source-bundle comparison against the
hardware-tested RGB stage found only the intended owner/bridge/header/test
changes plus the VM build path and stage logs.

The physical N100 was staged with candidate owner SHA-256
`d314345fd7523d5b6bd9c7ad784ab7a1aa9af331919154fda5e447ecd35c36df`
and bridge SHA-256
`80b97ea7bd21c021cdefaf0cce83b39fccb30c1d750df6562761c1aa529f39a3`.
The previously installed owner and bridge are backed up under
`/boot/home/native-rcs-v8-028/rollback/`, with SHA-256
`fdd85665ff0ee1989285fcf9e7796598529b94bf3d6ad8fb266c8a7232d2a075`
and `3c627564f538d62e87342e3da6968b4843170ee14044e1bc2c6d1a916c22680f`.
After a controlled reboot, both new on-disk hashes and the native device were
confirmed (`physical-state.log`).

The read-only owner probe, bootstrap, and original fixed draw passed
(`probe-bootstrap.log`). The new version-8 direct draw completed owner sequence
3 with 777/777 checked/matched pixels, exact hash `28616d2d`, and no reported
fault (`depth-draw.log`). The older version-7 combined draw then completed
sequence 4. Four compiler-backed Vulkan RGB triangle shapes also produced
their established exact hashes under the new owner (`v7-regression.log`).
The physical test lock was verified free afterward.

**Limit of this proof:** replacing z with 0.25 leaves the visible color image
unchanged with depth testing disabled. The version-8 request, validation,
submission, completion, and readback are verified, but the pixel output alone
does not prove that the GPU executed the new vertex arithmetic. A later test
must use a visibly different vertex program and matching independent image
expectations. The native Vulkan library still uses version 7; version 8 was
tested through the direct owner application in this checkpoint.
