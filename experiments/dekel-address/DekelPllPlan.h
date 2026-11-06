/*
 * Copyright 2026 RenkuOS GPU Development.
 * Distributed under the terms of the MIT License.
 *
 * Experimental integer-only HDMI PLL planning. No MMIO or driver integration.
 * Hardware constraints and divider order are informed by Linux i915's
 * intel_dpll_mgr.c, Copyright Intel Corporation, MIT License.
 */
#ifndef RENKU_EXPERIMENTAL_DEKEL_PLL_PLAN_H
#define RENKU_EXPERIMENTAL_DEKEL_PLL_PLAN_H

#include <stdint.h>

struct DekelHdmiPllPlan {
	uint32_t dcoKHz;
	uint32_t highSpeedDivider;
	uint32_t secondaryDivider;
	uint32_t feedbackPredivider;
	uint32_t feedbackInteger;
	uint32_t feedbackFraction;
};

// Calculate divider and 22-bit fractional feedback for non-SSC HDMI.
// Returns false rather than producing an unrepresentable or invalid plan.
static inline bool
PlanDekelHdmiPll(uint32_t pixelClockKHz, uint32_t referenceKHz,
	DekelHdmiPllPlan* plan)
{
	if (plan == 0 || pixelClockKHz == 0 || (referenceKHz != 19200
		&& referenceKHz != 24000 && referenceKHz != 38400))
		return false;

	const uint32_t highSpeed[] = {7, 5, 3, 2};
	for (unsigned i = 0; i < sizeof(highSpeed) / sizeof(highSpeed[0]); i++) {
		for (int secondary = 10; secondary >= 1; secondary--) {
			uint64_t dco = uint64_t(pixelClockKHz) * 5 * highSpeed[i]
				* uint32_t(secondary);
			if (dco < 7992000 || dco > 10000000)
				continue;

			const uint32_t predivider = 2;
			uint32_t integer = uint32_t(dco / (referenceKHz * predivider));
			if (integer > 255)
				return false;

			uint32_t denominator = referenceKHz * predivider;
			uint32_t remainder = uint32_t(dco % denominator);
			plan->dcoKHz = uint32_t(dco);
			plan->highSpeedDivider = highSpeed[i];
			plan->secondaryDivider = uint32_t(secondary);
			plan->feedbackPredivider = predivider;
			plan->feedbackInteger = integer;
			plan->feedbackFraction = uint32_t((uint64_t(remainder) << 22)
				/ denominator);
			return true;
		}
	}
	return false;
}

#endif
