# SPDX-License-Identifier: MIT
$(OUT)/dmar-native-module.o: tests/dmar-native-module.cpp tests/IntelDmarInventory.h tests/dmar-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/dmar-native-bridge.o: tests/dmar-native-bridge.cpp tests/dmar-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/intel_dmar_snapshot: $(OUT)/dmar-native-module.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/intel_dmar_bridge: $(OUT)/dmar-native-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/dmar-native-client: tests/dmar-native-client.cpp tests/dmar-native-protocol.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
