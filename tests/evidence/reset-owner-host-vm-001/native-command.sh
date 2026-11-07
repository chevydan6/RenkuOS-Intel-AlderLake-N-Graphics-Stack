set -eu
cd /boot/home/test-work
printf '%s
' 'e16414da6ea1264737687fa947c2c20efd667c32f9664e802d74e46f1e1fce1f  a33-native-owner-001.tar.gz' | sha256sum -c -
mkdir a33-native-owner-001
tar -xzf a33-native-owner-001.tar.gz -C a33-native-owner-001
cd a33-native-owner-001
sha256sum -c inputs.sha256 > inputs-check.log
uname -a
g++ --version
g++ -std=c++17 -O2 -Wall -Wextra -Werror -pthread -Iinputs inputs/owner-observation-test.cpp -o owner-test
./owner-test
mkdir build
make -f Makefile build/driver/kernel_cpp.o
cat > owner.mk <<'MAKE'
include Makefile
build/reset-owner.o: inputs/IntelMemoryOwner.cpp
	$(CXX) -Iinputs $(CPPFLAGS) $(CXXFLAGS) $(KERNELFLAGS) -Werror -c $< -o $@
build/intel_memory_owner_v7_candidate: build/reset-owner.o build/driver/kernel_cpp.o
	$(CXX) -shared -nostdlib -Wl,--no-undefined -Wl,-z,max-page-size=0x1000 -o $@ $^ -L/boot/system/develop/lib -l:_KERNEL_ /boot/system/develop/lib/haiku_version_glue.o
MAKE
make -f owner.mk build/intel_memory_owner_v7_candidate
sha256sum build/intel_memory_owner_v7_candidate
