# Roadmap

1. Preserve verified reconnaissance and initial patches; investigate RenkuOS
   source/policies/interfaces before changing the primary target.
2. Reproduce missing-pair and unsupported-hook tests before/after fixes. Compile
   paired components and retain baseline artifacts.
3. Complete GGTT/PTE-width/stolen-memory and initialization audit before a
   physical patched-driver trial; validate the BAR offset fix natively.
4. Establish stock display behavior on the actual 461c:46d1 system, then test
   reviewed display/memory fixes under an authorized recovery window. Introduce
   bounded kernel-owned scratch buffers before execution work.
5. Implement one validated Gen12 execution path with completion and recovery.
   This infrastructure is a prerequisite for useful 2D acceleration.
6. Add tested blits/fills and measure correctness, latency and memory overhead;
   integrate app_server separately when justified.
7. Evaluate Mesa Iris through an explicit kernel-interface capability matrix;
   reuse compiler/ISL/state code, preserve its language and license.
8. Hardware triangle/textures/shaders, then OpenGL coverage and stress.
9. Evaluate ANV/Vulkan only after stable memory, contexts and synchronization.

No milestone past component compilation/capability tests is complete.
