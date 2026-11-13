# Graphics architecture

Use the existing PCI driver → GART/shared areas → Intel accelerant → app_server
path for display. Keep device generation/display-version distinctions explicit.
Existing kernel/accelerant shared structs are an ABI: compile paired components
and avoid changing layouts without versioning and clone/client review.

For rendering, first design kernel-owned buffer handles with checked size,
alignment, range, ownership and mapping lifetime. Separate GGTT scanout from
per-context GPU virtual memory. Define CPU/GPU cache transitions and fences
before permitting submission. A completion result must distinguish success,
timeout, reset and lost context. Close/process death must drain work before DMA
memory is freed. Bounds and integer-overflow checks belong at the kernel boundary.

Initial GPU execution should be a kernel-constructed fixed blit between bounded
scratch surfaces, with a timeout, completion verification and guards around the
destination. Do not expose raw user batch submission, privileged registers, or
arbitrary physical/GPU addresses. Introduce reset/recovery before workload loops.
Only advertise a drawing hook when its submission and synchronization work.

The current legacy queue is not a Gen12 logical-context engine. Establish a
small Gen12-specific backend beneath the existing display driver; keep API state
and shader compilation in Mesa. Iris integration needs query, allocation,
mapping, GPU address space, context, synchronization and reset semantics. Choose
a native Haiku backend or a deliberately bounded DRM-compatible interface only
after enumerating actual required calls. A shim with successful no-op IOCTLs is
not a compatibility layer.

App_server's current local drawing path does not consume the old 2D hooks.
Desktop acceleration therefore requires a separately measured integration change;
working a blitter in a test client is only an intermediate result.
