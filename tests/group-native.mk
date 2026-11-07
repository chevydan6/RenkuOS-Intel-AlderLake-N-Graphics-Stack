# SPDX-License-Identifier: MIT
GROUP_HEADERS = tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100ForcewakeGroup.h tests/IntelN100ResetDomains.h tests/IntelN100BcsInventory.h tests/IntelN100GtAck.h tests/IntelN100PciPower.h tests/IntelApertureLeaseState.h
$(OUT)/group-owner.o: tests/IntelMemoryOwner.cpp $(GROUP_HEADERS)
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v6_candidate: $(OUT)/group-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/group-bridge.o: tests/group-native-bridge.cpp tests/group-native-protocol.h $(GROUP_HEADERS)
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_group_handshake_bridge: $(OUT)/group-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/group-native-client: tests/group-native-client.cpp tests/group-native-protocol.h $(GROUP_HEADERS)
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
