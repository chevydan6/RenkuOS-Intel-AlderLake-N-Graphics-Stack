# SPDX-License-Identifier: MIT
CC = gcc
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -Werror -ffreestanding -fno-builtin -mno-red-zone -fno-stack-protector
.PHONY: host-object native-object
host-object:
	python3 extract.py
	$(CC) $(CFLAGS) -Iinclude -MMD -c build/intel_ring_slice.c -o build/intel_ring_slice.o
	nm -u build/intel_ring_slice.o
native-object:
	python3 extract.py
	$(CC) $(CFLAGS) $(NATIVE_CPPFLAGS) -DRENKU_NATIVE -Iinclude -MMD -c build/intel_ring_slice.c -o build/intel_ring_slice.native.o

.PHONY: allocator-test target-objects
allocator-test:
	python3 verify-upstream.py
	mkdir -p build
	$(CC) -std=gnu11 -O1 -g -Wall -Wextra -Werror -Wno-unused-parameter -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude -Iupstream upstream/drm/drm_mm.c upstream/lib/rbtree.c tests/drm_mm_test.c $(SANITIZER_LDFLAGS) -o build/drm_mm_test
	UBSAN_OPTIONS=halt_on_error=1 ./build/drm_mm_test
target-objects:
	python3 target-build.py --source "$(RENKU_SOURCE)" --cc "$(CC)"

.PHONY: refcount-test
refcount-test:
	python3 verify-upstream.py
	mkdir -p build
	$(CC) -std=gnu11 -O1 -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare -fno-strict-overflow -fsanitize=address,undefined -fno-omit-frame-pointer -pthread -Iinclude -Iupstream upstream/lib/refcount.c tests/host_locks.c tests/refcount_test.c $(SANITIZER_LDFLAGS) -o build/refcount_test
	UBSAN_OPTIONS=halt_on_error=1 ./build/refcount_test

.PHONY: target-atomic-test
target-atomic-test: target-objects
	$(CC) -std=gnu11 -Wall -Wextra -Werror tests/native_atomic_runner.c build/target/native_atomic_probe.o -o build/native_atomic_test
	./build/native_atomic_test
