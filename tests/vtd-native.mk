# SPDX-License-Identifier: MIT
$(OUT)/vtd-native-module.o: tests/vtd-native-module.cpp tests/IntelDmarInventory.h tests/IntelVtdObservation.h tests/dmar-native-protocol.h tests/vtd-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/vtd-native-bridge.o: tests/vtd-native-bridge.cpp tests/vtd-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/intel_vtd_observer: $(OUT)/vtd-native-module.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/intel_vtd_bridge: $(OUT)/vtd-native-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/vtd-native-client: tests/vtd-native-client.cpp tests/vtd-native-protocol.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
