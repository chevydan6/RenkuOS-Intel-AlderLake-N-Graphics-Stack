# SPDX-License-Identifier: MIT
$(OUT)/gt-ack-owner.o: tests/IntelMemoryOwner.cpp tests/IntelMemoryOwner.h tests/IntelApertureLeaseState.h tests/IntelN100PciPower.h tests/IntelN100GtAck.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v2_candidate: $(OUT)/gt-ack-owner.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/gt-ack-bridge.o: tests/gt-ack-native-bridge.cpp tests/gt-ack-native-protocol.h tests/IntelMemoryOwner.h tests/IntelN100GtAck.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_gt_ack_bridge: $(OUT)/gt-ack-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/gt-ack-native-client: tests/gt-ack-native-client.cpp tests/gt-ack-native-protocol.h tests/IntelN100GtAck.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
