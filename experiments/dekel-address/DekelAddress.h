/*
 * Copyright 2026 RenkuOS GPU Development.
 * Distributed under the terms of the MIT License.
 *
 * Experimental arithmetic only. This is not connected to MMIO or the driver.
 * Register layout reference: Linux intel_dkl_phy_regs.h, Copyright 2022 Intel,
 * SPDX-License-Identifier: MIT.
 */
#ifndef RENKU_EXPERIMENTAL_DEKEL_ADDRESS_H
#define RENKU_EXPERIMENTAL_DEKEL_ADDRESS_H

#include <stdint.h>

struct DekelAddress {
	uint32_t aperture;
	uint32_t indexRegister;
	uint32_t indexValue;
};

// port is numbered 1 through 6; internalOffset is a four-bit bank plus a
// byte offset within the 4 KiB aperture. Only aligned 32-bit registers fit.
static inline bool
MakeDekelAddress(unsigned port, uint32_t internalOffset, DekelAddress* result)
{
	if (port < 1 || port > 6 || internalOffset > 0xffff
		|| (internalOffset & 3) != 0 || result == 0)
		return false;

	unsigned index = port - 1;
	result->aperture = 0x168000u + index * 0x1000u
		+ (internalOffset & 0xfffu);
	result->indexRegister = index < 4 ? 0x1010a0u : 0x1010a4u;
	result->indexValue = ((internalOffset >> 12) & 0xfu)
		<< (8 * (index % 4));
	return true;
}

#endif
