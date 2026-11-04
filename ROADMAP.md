# Roadmap

1. Preserve verified reconnaissance and initial patches; investigate RenkuOS
   source/policies/interfaces before changing the primary target.
2. Reproduce missing-pair and unsupported-hook tests before/after fixes. Compile
   paired components and retain baseline artifacts.
3. Validate unchanged native display first on the actual 461c:46d1 system;
   separate the ID-table change from later rendering changes.
4. Audit GGTT/stolen memory and introduce bounded kernel-owned scratch buffers.
5. Implement one validated Gen12 execution path with completion and recovery.
   This infrastructure is a prerequisite for useful 2D acceleration.
6. Add tested blits/fills and measure correctness, latency and memory overhead;
   integrate app_server separately when justified.
7. Evaluate Mesa Iris through an explicit kernel-interface capability matrix;
   reuse compiler/ISL/state code, preserve its language and license.
8. Hardware triangle/textures/shaders, then OpenGL coverage and stress.
9. Evaluate ANV/Vulkan only after stable memory, contexts and synchronization.

No milestone past component compilation/capability tests is complete.
