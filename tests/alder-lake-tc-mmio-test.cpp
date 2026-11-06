/* SPDX-License-Identifier: MIT */
#include "AlderLakeTypeCMmio.h"
#include <stdio.h>
#include <vector>

int main() {
	unsigned errors = 0;
	std::vector<uint32> words(0x16b000 / 4, 0);
	volatile uint8* base = (volatile uint8*)words.data();
	AlderLakeTypeCMmio mmio(base, words.size() * 4, 2, 1, true);
	AlderLakeTypeCIO io = mmio.Ops(); uint32 value = 0;
	if (io.write(io.cookie, 0x64400, 0x1234) != B_OK
		|| io.read(io.cookie, 0x64400, &value) != B_OK || value != 0x1234) errors++;
	if (io.write(io.cookie, 0x46110, 0x80000000U) != B_OK
		|| io.write(io.cookie, 0x164280, 0x2000) != B_OK
		|| io.write(io.cookie, 0x4610c, 0) != B_BAD_VALUE) errors++;
	if (io.write(io.cookie, 0x161504, 1) != B_BAD_VALUE
		|| io.write(io.cookie, 0x64300, 1) != B_BAD_VALUE
		|| io.write(io.cookie, 0x64401, 1) != B_BAD_VALUE
		|| io.read(io.cookie, 0x169200, &value) != B_BAD_VALUE) errors++;
	if (io.write(io.cookie, 0x1010a0, 0x300) != B_BAD_VALUE
		|| io.write(io.cookie, 0x1010a0, 0x201) != B_BAD_VALUE
		|| io.write(io.cookie, 0x1010a0, 0x200) != B_OK) errors++;
	if (io.write(io.cookie, 0x169200, 0x9876) != B_OK
		|| io.read(io.cookie, 0x169200, &value) != B_OK || value != 0x9876
		|| io.read(io.cookie, 0x16936c, &value) != B_OK
		|| io.write(io.cookie, 0x16936c, 1) != B_BAD_VALUE) errors++;
	if (io.write(io.cookie, 0x1010a0, 0) != B_OK
		|| io.write(io.cookie, 0x1692c0, 0x55) != B_OK
		|| io.write(io.cookie, 0x169200, 0x55) != B_BAD_VALUE) errors++;
	AlderLakeTypeCMmio unverified(base, words.size() * 4, 2, 1, false);
	AlderLakeTypeCIO no = unverified.Ops();
	if (no.write(no.cookie, 0x64400, 1) != B_BAD_VALUE) errors++;
	AlderLakeTypeCMmio shortMapping(base, 0x1000, 2, 1, true);
	no = shortMapping.Ops();
	if (no.read(no.cookie, 0x64400, &value) != B_BAD_VALUE) errors++;
	AlderLakeTypeCMmio badPort(base, words.size() * 4, 5, 1, true);
	no = badPort.Ops();
	if (no.write(no.cookie, 0x64400, 1) != B_BAD_VALUE) errors++;
	const uint32 planeRegisters[] = {0x71180,0x711cc,0x71188,0x711a4,0x7119c,0x7127c,0x71240};
	for (uint32 offset : planeRegisters) {
		words[offset/4]=0x1234;
		if(io.read(io.cookie,offset,&value)!=B_OK || value!=0x1234
			|| io.write(io.cookie,offset,0)!=B_BAD_VALUE
			|| io.read(io.cookie,offset-0x1000,&value)!=B_BAD_VALUE)errors++;
	}
	if (errors) { fprintf(stderr, "%u MMIO adapter cases failed\n", errors); return 1; }
	puts("16 MMIO range, bank, device and access-control cases passed; RAM only");
	return 0;
}
