/* SPDX-License-Identifier: MIT */
#include "DekelHdmiSignal.h"
#include <stdio.h>

int main() {
	unsigned errors = 0;
	const uint32 swing[] = {7, 6, 4, 2, 0, 0, 0, 0, 0, 0};
	const uint32 emphasis[] = {0, 0, 0, 0, 0, 5, 6, 7, 8, 10};
	for (unsigned preset = 0; preset < 10; preset++) {
		for (unsigned workaround = 0; workaround < 2; workaround++) {
			DekelHdmiSignalPlan plan = {};
			if (BuildDekelHdmiSignalPlan(preset, 148500, workaround, &plan) != B_OK) {
				errors++; continue;
			}
			for (unsigned lane = 0; lane < 2; lane++) {
				unsigned i = lane * 4; uint32 offset = lane * 0x1000;
				uint32 signal = swing[preset] | (emphasis[preset] << 8);
				if (plan.updates[i].internalOffset != 0xd00 + offset
					|| plan.updates[i].mask != 0xffffffffU || plan.updates[i].value != 0
					|| plan.updates[i+1].internalOffset != 0x2c0 + offset
					|| plan.updates[i+2].internalOffset != 0x2c4 + offset
					|| plan.updates[i+1].mask != 0x3ff07 || plan.updates[i+2].mask != 0x3ff07
					|| plan.updates[i+1].value != signal || plan.updates[i+2].value != signal
					|| plan.updates[i+3].internalOffset != 0x2c8 + offset
					|| plan.updates[i+3].mask != (0x7cU | (workaround ? 0x1000U : 0))
					|| plan.updates[i+3].value != (lane ? 0x78U : 0x40U)) errors++;
			}
		}
	}
	DekelHdmiSignalPlan plan = {};
	if (BuildDekelHdmiSignalPlan(9, 340000, true, &plan) != B_OK) errors++;
	if (BuildDekelHdmiSignalPlan(10, 148500, false, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiSignalPlan(0, 0, false, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiSignalPlan(0, 340001, false, &plan) != B_BAD_VALUE
		|| BuildDekelHdmiSignalPlan(0, 148500, false, NULL) != B_BAD_VALUE) errors++;
	if (errors) { fprintf(stderr, "%u signal plan cases failed\n", errors); return 1; }
	puts("45 Dekel HDMI signal-plan and input cases passed; no hardware access");
	return 0;
}
