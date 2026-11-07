# SPDX-License-Identifier: MIT
$(OUT)/bcs-owner.o: tests/IntelMemoryOwner.cpp tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100BcsInventory.h tests/IntelN100GtAck.h tests/IntelN100PciPower.h tests/IntelApertureLeaseState.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v4_candidate: $(OUT)/bcs-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/bcs-bridge.o: tests/bcs-native-bridge.cpp tests/bcs-native-protocol.h tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100BcsInventory.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_bcs_inventory_bridge: $(OUT)/bcs-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/bcs-native-client: tests/bcs-native-client.cpp tests/bcs-native-protocol.h tests/IntelN100Forcewake.h tests/IntelN100BcsInventory.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
