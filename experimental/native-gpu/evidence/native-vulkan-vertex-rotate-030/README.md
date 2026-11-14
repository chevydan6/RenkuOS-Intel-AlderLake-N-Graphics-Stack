# Visible N100 vertex execution through native Vulkan

`triangle-rotate-halfturn.vert.spvasm` negates vertex x/y and preserves z/w.
SPIRV-Tools assembled and validated its 476-byte Vulkan 1.0 module (SHA-256
`04d8daf34c532b608bb832772f2e451fdb3a02661c72b78e9589ec2b5e46d292`).
The Haiku-native Mesa 26.3.0 compiler emitted 176 bytes of N100 machine code
(SHA-256 `2701c69fca3b305af0f1feea0c1c7fe750fc7c99861cf1fafe4f85d6ca8e0780`),
with zero scratch and relocations. GRF use, dispatch start, URB read/entry,
and dispatch mode match the existing fixed vertex state. The fragment binary
remained byte identical. See `compiler-results.log` and the retained binaries.

The owner admits only the exact 176-byte program or the existing 192-byte
depth program, each with zero padding through the full 4 KiB BO. For the
half-turn it validates the original vertex resource, builds rotated CPU
reference geometry, and checks the GPU target against that geometry. The
Vulkan compiler gate accepts the same exact 176-byte machine program, uploads
it to a third BO, and submits owner request version 8. The original vertex
program continues to use version 7.

Before installing the candidate owner, the direct rotate test was rejected at
stage 0 with sequence 0, as expected. The known-good version-8 owner was saved
under `/boot/home/native-vertex-rotate-030/rollback/`. Only the owner binary
was replaced; the bridge binary was retained. The candidate owner SHA-256 is
`16408adc09d8036d03dd9f07287308893da8d64f62fe8c8bd5d60f915876a900`.
After a controlled reboot, the read-only probe, bootstrap, and fixed draw
passed. VM owner and Vulkan builds passed; see `vm-owner-build.log` and
`vm-vulkan-build.log`.

The physical direct rotate draw completed with 777/777 checked pixels and
hash `77323fd5`. An independent fixed-pipeline draw of the corresponding
opposite triangle produced the same hash (`fixed-opposite-baseline.log`). Through the Vulkan application,
version-8 owner sequence 17 completed without fault and returned the same
full-image hash. The application also checked a pixel newly covered by the
rotation and one cleared by it. The depth program (version 8), four RGB
triangle shapes (version 8), the original vertex program (version 7), and a
two-draw version-7 batch all passed. Raw traces are in `physical.log`; current
binaries and rollback hashes are in `installed-hashes.log`.

This proves execution of the bounded half-turn vertex arithmetic for the
tested triangle and native submission path. The driver still admits a very
small set of exact shader programs; arbitrary Vulkan shaders and Vulkan
conformance remain future work.
