/* SPDX-License-Identifier: MIT */
#include "DekelHdmiPll.h"
#include <stdio.h>

int main() {
	unsigned errors = 0;
	DekelHdmiPllPlan plan = {};
	if (BuildDekelHdmiPllPlan(148500, 19200, &plan) != B_OK
		|| plan.dcoKHz != 8910000 || plan.highSpeedDivider != 3
		|| plan.secondaryDivider != 4) errors++;
	const uint32 expectedOffsets[] = {0x212c, 0x20d8, 0x20d4, 0x2200,
		0x2204, 0x2210, 0x2214, 0x2218};
	const uint32 expectedValues[] = {0x100, 0x500, 0x15400, 0x842e8,
		0x1c004f, 0x20002000, 0x42000000, 74};
	for (unsigned i = 0; i < 8; i++) {
		if (plan.updates[i].internalOffset != expectedOffsets[i]
			|| plan.updates[i].value != expectedValues[i]
			|| (plan.updates[i].value & ~plan.updates[i].mask) != 0) errors++;
	}
	if (BuildDekelHdmiPllPlan(594000, 24000, &plan) != B_OK
		|| plan.dcoKHz != 8910000 || plan.highSpeedDivider != 3
		|| plan.secondaryDivider != 1 || plan.updates[3].value != 0x842b9
		|| plan.updates[6].value != 0x68000000U) errors++;
	if (BuildDekelHdmiPllPlan(148500, 38400, &plan) != B_OK
		|| plan.updates[3].value != 0x84274
		|| plan.updates[5].value != 0x40002000U) errors++;
	if (BuildDekelHdmiPllPlan(0, 19200, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiPllPlan(148500, 25000, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiPllPlan(28100, 19200, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiPllPlan(0xffffffffU, 19200, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiPllPlan(148500, 19200, NULL) != B_BAD_VALUE) errors++;
	const uint32 references[] = {19200, 24000, 38400};
	for (unsigned r = 0; r < 3; r++) {
		for (uint32 clock = 25000; clock <= 600000; clock += 100) {
			if (BuildDekelHdmiPllPlan(clock, references[r], &plan) != B_OK)
				continue;
			uint32 integer = plan.updates[3].value & 0xff;
			uint32 fraction = (plan.updates[6].value >> 8) & 0x3fffff;
			uint64 scaled = uint64(references[r]) * 2
				* ((uint64(integer) << 22) | fraction);
			uint32 actual = uint32((scaled >> 22) / (5
				* plan.highSpeedDivider * plan.secondaryDivider));
			if (actual > clock || clock - actual > 1) errors++;
			for (unsigned i = 0; i < 8; i++)
				if ((plan.updates[i].value & ~plan.updates[i].mask) != 0) errors++;
		}
	}
	if (errors) { fprintf(stderr, "%u Dekel PLL cases failed\n", errors); return 1; }
	puts("Dekel HDMI PLL fixtures and 17,253 frequency inputs passed; no hardware access");
	return 0;
}
