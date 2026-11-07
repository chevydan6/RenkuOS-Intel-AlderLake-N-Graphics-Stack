# SPDX-License-Identifier: MIT
$(OUT)/ppgtt-native-module.o: tests/ppgtt-native-module.cpp tests/IntelN100KernelPages.h tests/IntelN100PpgttTree.h tests/IntelN100Ppgtt.h tests/ppgtt-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/ppgtt-native-bridge.o: tests/ppgtt-native-bridge.cpp tests/ppgtt-native-protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/intel_ppgtt_test: $(OUT)/ppgtt-native-module.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/intel_ppgtt_bridge: $(OUT)/ppgtt-native-bridge.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/ppgtt-native-client: tests/ppgtt-native-client.cpp tests/ppgtt-native-protocol.h
	$(CXX) -std=c++17 -O2 -Wall -Wextra -Werror -Itests $< -o $@
