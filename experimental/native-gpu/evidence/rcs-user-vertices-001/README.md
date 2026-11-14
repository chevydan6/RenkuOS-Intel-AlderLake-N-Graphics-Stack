# Bounded RCS user vertex candidate, 2026-11-14

This candidate adds a version 2 `DRAW` request that accepts three position
`vec4`s in the first 48 bytes of the existing `native_gpu_render_command`
output array. The owner validates exact 1/16-step x/y values, z=0.5, w=1,
positive winding, and a useful raster area before copying the vertices to its
private GPU page. The existing batch, shaders, addresses, surfaces, and page
ownership remain private. The command size and version 1 semantics do not
change. The output array becomes the 32x32 pixel image after execution.

The [native build log](native-build.log) ends with
`NATIVE_GPU_EXECUTION_BUILD_PASSED`. The [native smoke result](native-smoke.log)
passes validation, image construction, and readback checks using CPU fixtures;
it is **not** hardware draw proof. [Owner hashes](owner-hashes.txt) show the
candidate owner (`1e188b4e…`) differs from the still installed known-good
version 1 owner (`b5e15f1a…`).

Running the new client against the installed version 1 owner produced the
[expected rejection](old-owner-rejection.log): ioctl failure, no submission
sequence or completion, and no hardware draw claim. The candidate owner has
not been installed. The next hardware gate is to preserve the installed owner,
verify exclusive physical access and recovery power control, install only the
candidate owner, reboot, and run both the version 1 regression and version 2
draw/readback tests. Roll back to the saved owner on any boot or regression
failure.
