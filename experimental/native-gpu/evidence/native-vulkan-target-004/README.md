# Fixed Vulkan render target objects on N100, 2026-11-14

The native Vulkan library now creates an image view for its fixed 32×32 BGRA
image, a single-color render pass, and a framebuffer that connects them. A
33-pixel-wide framebuffer is rejected. The probe first failed at link time
for seven missing object entry points; after implementation, 16 consecutive
[physical N100 probes](repeat-and-draw.log) passed. The established v6
public-shader draw then passed with 654/654 expected pixels. No owner or base
kernel code changed.

The final library SHA-256 is
`1b167c1f1241bd92b173fcc4d5704c72dfc051dad719e29ac480db10ea3702af`;
the probe SHA-256 is
`a245e6c2363f2b6430b4e4f27a3c3c5dc043858e478e774f54397dce20aa0ae2`.
The [binary hashes](binaries.sha256) include the physical paths.

The saved GCC depfiles and [manifest](manifest.json) inventory direct compile
inputs: 15 Khronos headers (1,459,702 bytes), one Vulkan library source file
(25,773 bytes), two native ABI headers (4,543 bytes), and two probe inputs
(12,758 bytes), totaling **20 files and 1,502,776 bytes**. This is not the
complete graphics-stack closure or a model-token count.

The render target objects are ready to identify the destination for a future
draw. No Vulkan command buffer has executed, and no Vulkan image pixels have
been rendered or read back yet.
