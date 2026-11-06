# SPDX-License-Identifier: MIT
# Explicit test targets; never part of production installation.
PROBE := tests/native-owner-probe
$(OUT)/terra_owner_probe.o: $(PROBE)/terra_owner_probe.cpp $(PROBE)/protocol.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -I$(PROBE) -c $< -o $@
$(OUT)/terra_owner_probe: $(OUT)/terra_owner_probe.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-soname,terra_owner_probe \
		-Wl,-z,max-page-size=0x1000 -o $@ $^ \
		-L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
$(OUT)/terra_owner_probe_client: $(PROBE)/client.cpp $(PROBE)/protocol.h
	$(CXX) -std=c++17 -Wall -Wextra -Werror -I$(PROBE) $< -o $@
