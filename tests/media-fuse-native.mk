# SPDX-License-Identifier: MIT
$(OUT)/media-fuse-owner.o: tests/IntelMemoryOwner.cpp tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100BcsInventory.h tests/IntelN100GtAck.h tests/IntelN100PciPower.h tests/IntelApertureLeaseState.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v5_candidate: $(OUT)/media-fuse-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/media-fuse-bridge.o: tests/media-fuse-native-bridge.cpp tests/media-fuse-native-protocol.h tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_media_fuse_bridge: $(OUT)/media-fuse-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/media-fuse-native-client: tests/media-fuse-native-client.cpp tests/media-fuse-native-protocol.h tests/IntelN100Forcewake.h tests/IntelN100ResetDomains.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
