# SPDX-License-Identifier: MIT
$(OUT)/kernel-pages-compile.o: tests/kernel-pages-compile.cpp tests/IntelN100KernelPages.h tests/IntelN100PpgttTree.h tests/IntelN100Ppgtt.h tests/IntelDmarReserved.h tests/IntelDmarInventory.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -Itests -c $< -o $@
$(OUT)/kernel-pages-compile.so: $(OUT)/kernel-pages-compile.o $(OUT)/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
