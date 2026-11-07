Resident wired-page integration test, build and runtime procedure

From prepared native bundle after placing fixture headers/sources in tests/:
make -f Makefile -f tests/ppgtt-native.mk build/intel_ppgtt_test build/intel_ppgtt_bridge build/ppgtt-native-client
Record SHA256 for exact sources and binaries. Terra design/source review accepted.
Install module only as kernel/bus_managers/intel_ppgtt_test, bridge as drivers/bin/
intel_ppgtt_bridge, and dev/misc/intel_ppgtt_test symlink to../../bin/intel_ppgtt_bridge
under isolated non-packaged test tree. Refuse to overwrite existing files. No
production graphics modules are replaced. Hold canonical locks throughout test.
Run build/ppgtt-native-client in visible NanoKVM terminal with60second external
observer/recovery. Expected version1,status0,completed3,verified42,live0,state0,
cached_result identical. absent count is supporting evidence, not required pass.
Capture before/after PCI,loadedpaths,hashes,display,processes,syslog and exit.
Never reinterpret build success as kernel runtime. Resident module remains until
reboot, retaining all state on failure. Do not unload or manually reclaim areas.
One-shot repeated requests return cached result; new run requires controlled boot.
The test uses CPU physical mappings only; it does not establish GPU DMA/IOMMU,
cache visibility, hardware table publication, GPU execution or acceleration.
