// SPDX-License-Identifier: MIT
// Native VM probe for clone_area lifetime using ordinary RAM, not device MMIO.
#include <OS.h>
#include <stdio.h>
#include <stdint.h>

int main()
{
	void* originalAddress = NULL;
	area_id original = create_area("terra owner surrogate", &originalAddress,
		B_ANY_ADDRESS, B_PAGE_SIZE, B_NO_LOCK,
		B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA);
	if (original < B_OK) {
		fprintf(stderr, "create_area failed: %ld\n", (long)original);
		return 1;
	}
	void* firstAddress = NULL;
	void* secondAddress = NULL;
	area_id first = clone_area("terra clone one", &firstAddress, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, original);
	if (first < B_OK) {
		fprintf(stderr, "first clone failed: %ld\n", (long)first);
		delete_area(original);
		return 1;
	}
	area_id second = clone_area("terra clone two", &secondAddress, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, original);
	if (second < B_OK) {
		fprintf(stderr, "second clone failed: %ld\n", (long)second);
		delete_area(first);
		delete_area(original);
		return 1;
	}
	volatile uint32_t* originalWord = (volatile uint32_t*)originalAddress;
	volatile uint32_t* firstWord = (volatile uint32_t*)firstAddress;
	volatile uint32_t* secondWord = (volatile uint32_t*)secondAddress;
	*originalWord = 0x5a17c0de;
	bool pass = *firstWord == 0x5a17c0de && *secondWord == 0x5a17c0de;
	if (delete_area(original) != B_OK)
		pass = false;
	if (*firstWord != 0x5a17c0de || *secondWord != 0x5a17c0de)
		pass = false;
	*firstWord = 0x2c81b4e9;
	if (*secondWord != 0x2c81b4e9)
		pass = false;
	if (delete_area(first) != B_OK)
		pass = false;
	if (*secondWord != 0x2c81b4e9)
		pass = false;
	if (delete_area(second) != B_OK)
		pass = false;
	puts(pass ? "PASS: RAM clones survived owner and first clone deletion"
		: "FAIL: RAM clone visibility or cleanup");
	return pass ? 0 : 1;
}
