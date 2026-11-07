# SPDX-License-Identifier: MIT
$(OUT)/forcewake-owner.o: tests/IntelMemoryOwner.cpp tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h tests/IntelN100GtAck.h tests/IntelN100PciPower.h tests/IntelApertureLeaseState.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v3_candidate: $(OUT)/forcewake-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/forcewake-bridge.o: tests/forcewake-native-bridge.cpp tests/forcewake-native-protocol.h tests/IntelMemoryOwner.h tests/IntelN100Forcewake.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_forcewake_bridge: $(OUT)/forcewake-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/forcewake-native-client: tests/forcewake-native-client.cpp tests/forcewake-native-protocol.h tests/IntelN100Forcewake.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
