# SPDX-License-Identifier: MIT
RESET_HEADERS = tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100ForcewakeGroup.h tests/IntelN100ResetDomains.h tests/IntelN100BcsReset.h tests/IntelN100ResetSafety.h tests/IntelN100BcsInventory.h tests/IntelN100GtAck.h tests/IntelN100PciPower.h tests/IntelApertureLeaseState.h
$(OUT)/reset-owner.o: tests/IntelMemoryOwner.cpp $(RESET_HEADERS)
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v7_candidate: $(OUT)/reset-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/reset-bridge.o: tests/reset-native-bridge.cpp tests/reset-native-protocol.h $(RESET_HEADERS)
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_bcs_reset_bridge: $(OUT)/reset-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/reset-native-client: tests/reset-native-client.cpp tests/reset-native-protocol.h $(RESET_HEADERS)
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
