# Astra verification of Qwen offline regression outputs

Qwen output directory: `build/qwen-offline-validation-002/`. Astra checked copied
sources byte-for-byte against the current project, checked ASan/UBSan linkage,
and ran each supplied binary. All passed. Because the supplied directory lacked
complete run/exit records, Astra also rebuilt the three programs with C++17,
warnings-as-errors and ASan/UBSan, recording commands, compiler, input/binary
hashes and compile/run exits in `review.json` and adjacent logs.

Results: 32 nonce packet, 2150 PPGTT encoding/range and 69 owned PPGTT tree cases.
These validate offline encoding/construction only; no GPU execution is claimed.
