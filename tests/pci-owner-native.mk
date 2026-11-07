# SPDX-License-Identifier: MIT
# Isolated candidate; the currently loaded owner is not replaced by this build.
$(OUT)/pci-owner-candidate.o: tests/IntelMemoryOwner.cpp tests/IntelMemoryOwner.h tests/IntelApertureLeaseState.h tests/IntelN100PciPower.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_memory_owner_v1_candidate: $(OUT)/pci-owner-candidate.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/pci-owner-bridge.o: tests/pci-owner-native-bridge.cpp tests/pci-owner-native-protocol.h tests/IntelMemoryOwner.h tests/IntelN100PciPower.h
	$(CXX) -Itests $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
$(OUT)/intel_pci_owner_bridge: $(OUT)/pci-owner-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/pci-owner-native-client: tests/pci-owner-native-client.cpp tests/pci-owner-native-protocol.h tests/IntelN100PciPower.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
