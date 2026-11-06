/* SPDX-License-Identifier: MIT */
#include "DekelAddress.h"
#include <stdio.h>

int main()
{
	DekelAddress a = {};
	unsigned failures = 0;
	if (!MakeDekelAddress(2, 0x2200, &a) || a.aperture != 0x169200
		|| a.indexRegister != 0x1010a0 || a.indexValue != 0x200)
		failures++;
	if (!MakeDekelAddress(5, 0xf014, &a) || a.aperture != 0x16c014
		|| a.indexRegister != 0x1010a4 || a.indexValue != 0xf)
		failures++;
	if (!MakeDekelAddress(6, 0x1014, &a) || a.aperture != 0x16d014
		|| a.indexRegister != 0x1010a4 || a.indexValue != 0x100)
		failures++;
	if (MakeDekelAddress(0, 0, &a) || MakeDekelAddress(7, 0, &a)
		|| MakeDekelAddress(1, 0x10000, &a)
		|| MakeDekelAddress(1, 2, &a) || MakeDekelAddress(1, 0, 0))
		failures++;
	for (unsigned port = 1; port <= 6; port++) {
		for (unsigned bank = 0; bank < 16; bank++) {
			if (!MakeDekelAddress(port, bank * 0x1000 + 0xffc, &a)
				|| a.aperture != 0x168000 + (port - 1) * 0x1000 + 0xffc
				|| a.indexValue != bank << (8 * ((port - 1) % 4)))
				failures++;
		}
	}
	if (failures != 0) {
		fprintf(stderr, "%u Dekel address checks failed\n", failures);
		return 1;
	}
	puts("104 Dekel address and input checks passed; no hardware access");
	return 0;
}
