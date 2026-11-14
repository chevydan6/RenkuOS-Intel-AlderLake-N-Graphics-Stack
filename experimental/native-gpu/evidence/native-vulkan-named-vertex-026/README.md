# Equivalent vertex SPIR-V accepted by the N100 compiler-backed pipeline

The compiler-backed Vulkan pipeline previously required the vertex SPIR-V
words to equal one fixture. Its Intel compiler already checked that the
emitted vertex machine code exactly matched the owner-approved N100 program.
The byte-for-byte SPIR-V gate is now removed from this alternate path; the
compiled machine-code gate remains. The default noncompiler build retains its
fixture gate.

The regression module inserts a valid `OpName %main "entry"` debug instruction
into the original Vulkan 1.0 vertex module. SPIRV-Tools accepted it. The VM
API probe was run before the change and failed at the expected pipeline gate
with exit code 13; after the change it reached the owner allocation boundary
with the same validated 128-byte vertex machine code. The compiler, fence, and
command-reset VM probes also passed.

The physical N100 tested an isolated library SHA-256
`7d01a06b0ad78933c884ba1a48b54f59b9a7ccf0cb2ea990ee91758bf4bbd2ac`
and triangle probe SHA-256
`ac85ee525e12c62add62f88bd3be95f752704fce6f0e331b457992041a943f2f`
under `/boot/home/native-vulkan-named-vs-026`. The installed kernel owner and
previous Vulkan checkpoint were unchanged.

The [trace](trace.log) shows owner sequence 246, 777/777 checked/matched
pixels, no reported GPU fault, and exact image hash `6f2bc92d`. Its SHA-256 is
`0c56902ebf019504b66241a4018266369eb2253a0ba5279fee3a9a1f7bd67c42`.
The [matrix](matrix.log) ran the named module and previous two-submit batch
through all four shapes, obtaining the known hashes `6f2bc92d`, `a4e13743`,
`ce445d05`, and `fffe3dc5`. Its SHA-256 is
`4e9d33f427bfa60d81ec87d2824fb61a0200cb82f3de482568ddbdf890042ecc`.

This accepts equivalent vertex modules with identical compiled machine code;
it does not add a new vertex instruction sequence or general shader support.
