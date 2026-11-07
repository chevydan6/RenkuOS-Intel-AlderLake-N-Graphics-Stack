Host mocked VM tests (from gpu-driver root):
g++ -std=c++17 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all -I tests/kernel-pages-host -I workstreams/gpu-next tests/n100-kernel-pages-test.cpp -o /tmp/n100-kernel-pages-test
/tmp/n100-kernel-pages-test

Mock VM APIs only. Reset marks the end of a simulated boot and reclaims test RAM;
production Retained/Ambiguous records stay resident until safe cleanup or reboot.
Native compile/link only: copy headers and kernel-pages-compile.cpp/native.mk
to prepared bundle tests/, then make -f Makefile -f tests/kernel-pages-native.mk
build/kernel-pages-compile.so . Artifact has no module entrypoint and must not be
installed. Actual kernel allocation/runtime needs a separate resident fixture.
