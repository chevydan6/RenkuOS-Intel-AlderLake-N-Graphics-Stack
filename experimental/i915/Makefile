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
