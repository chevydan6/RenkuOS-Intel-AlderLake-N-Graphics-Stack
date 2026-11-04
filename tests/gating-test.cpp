// SPDX-License-Identifier: MIT
// Compile the real kernel function with RAM registers and test-local logging.
#include <stdio.h>
#include <string.h>
#include "power.h"

#undef dprintf
#define dprintf(...) ((void)0)
#ifndef GATING_SOURCE
#define GATING_SOURCE "power.cpp"
#endif
#include GATING_SOURCE
#undef dprintf

static uint32 sRegisters[0x50000 / sizeof(uint32)];
static unsigned sChecks;
static unsigned sFailures;

static void
check(bool pass, const char* label)
{
	sChecks++;
	if (!pass) {
		sFailures++;
		fprintf(stderr, "FAIL %s\n", label);
	}
}

int
main()
{
	pci_info pci = {};
	intel_shared_info shared = {.device_type = INTEL_MODEL_ALDM};
	intel_info info = {.device_type = INTEL_MODEL_ALDM};
	info.pci = &pci;
	info.shared_info = &shared;
	info.registers = reinterpret_cast<addr_t>(sRegisters);
	info.device_type = INTEL_MODEL_ALDM;
	check(intel_en_gating(info) == B_NOT_SUPPORTED,
		"Alder Lake gating unsupported");
	bool untouched = true;
	for (uint32 value : sRegisters)
		untouched &= value == 0;
	check(untouched, "Alder Lake gating registers preserved");

	// Positive control detects a test that accidentally suppresses all writes.
	memset(sRegisters, 0, sizeof(sRegisters));
	info.device_type = INTEL_MODEL_SNBG;
	check(intel_en_gating(info) == B_OK, "Sandy Bridge gating retained");
	check(sRegisters[0x42020 / sizeof(uint32)]
		== ((1U << 28) | (1U << 7) | (1U << 5)),
		"Sandy Bridge gating write retained");
	check(sRegisters[0x7408 / sizeof(uint32)] == 0x10,
		"legacy common gating write retained");
	printf("%u gating checks, %u failures; simulated register RAM only\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
