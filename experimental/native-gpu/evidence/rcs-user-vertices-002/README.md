# Physical N100 user-vertex RCS regression, 2026-11-14

The version 2 owner (`1e188b4ef7b46125278833c92af99a5de924075ea6e32da42d17e31e84b42dc9`)
was installed after preserving the known-good version 1 owner
(`b5e15f1aebfdaf46ee41e7d1ace683d7e10227e671208cd5f70257a8ce92183c`)
at `/boot/home/native-rcs-userdraw-006/known-good-intel_memory_owner`.
Only the owner binary changed; [bridge-hash.txt](bridge-hash.txt) records the
installed bridge. The N100 (`8086:46d1`, revision 0) rebooted, passed a
read-only probe, then explicitly bootstrapped RCS. The complete native build
and CPU smoke suite were rerun after adding the malformed-input client;
[the build log](native-build-after-invalid-client.log) ends with
`NATIVE_GPU_EXECUTION_BUILD_PASSED`, and the rebuilt owner hash matches the
installed owner.

The [version 1 regression](v1-post-v2-64/metadata.json) passed all 64 fixed
shader draws, preserving the four established pixel hashes and consecutive
completion sequences through 70. The [version 2 regression](v2-64/metadata.json)
then passed 64 user-vertex draws across four distinct triangle positions, with
all checked pixels matching, four distinct pixel hashes, and consecutive
completion sequences through 134. [Raw v2 reports](v2-64/draws.raw.log) and
[structured v2 records](v2-64/draws.jsonl) retain every request. A subsequent
[four-draw version 1 check](v1-after-v2/metadata.json) passed through sequence
138.

Three malformed version 2 requests (NaN coordinate, non-unit w, and zero-area
edge) were [rejected](invalid-rejection.log) with no submission or image output.
The [next valid draw](post-invalid-valid.log) passed at sequence 140; an
intermediate valid draw at sequence 139 was observed before the recorded
rejection run. The [final probe](final-probe.log) passed. Installed and backup
binary hashes are in [installed-hashes.txt](installed-hashes.txt).

SHA-256 of the v1 64-draw JSONL and raw log:
`ecc83d7b9847a24381eaaec913a75c2335e2d0c6497702e8ca5885690a7bb6b1`,
`c50f61dd10eac14c2e5c8723a6db28811a1dbdfbc19e1bf5784e53aba38438c4`.
SHA-256 of the v2 64-draw JSONL and raw log:
`8b1512d77e643626600ba8ecc56f0d4b4b515f6d679b9446f1c482283fc73626`,
`fe095cbe2c71f39d2c642f33987d723687c5cb292e8d3da5bff5a66023922af3`.

This proves a bounded user-controlled vertex upload, RCS execution, completion,
GPU-buffer rendering, and CPU readback on physical N100 hardware. It does not
provide general buffer objects, user command buffers, fences, SPIR-V shaders,
or Vulkan API support. The Govee outlet was manually restored after a separate
BLE pairing attempt; no remote Govee control key was acquired.
