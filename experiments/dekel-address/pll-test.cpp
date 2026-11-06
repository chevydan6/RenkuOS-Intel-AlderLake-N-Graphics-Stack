/* SPDX-License-Identifier: MIT */
#include "DekelPllPlan.h"
#include <stdio.h>

int main()
{
	DekelHdmiPllPlan p = {};
	unsigned failures = 0;
	if (!PlanDekelHdmiPll(148500, 19200, &p) || p.dcoKHz != 8910000
		|| p.highSpeedDivider != 3 || p.secondaryDivider != 4
		|| p.feedbackPredivider != 2 || p.feedbackInteger != 232
		|| p.feedbackFraction != 131072)
		failures++;
	if (!PlanDekelHdmiPll(594000, 24000, &p) || p.dcoKHz != 8910000
		|| p.highSpeedDivider != 3 || p.secondaryDivider != 1
		|| p.feedbackPredivider != 2 || p.feedbackInteger != 185
		|| p.feedbackFraction != 2621440)
		failures++;
	if (!PlanDekelHdmiPll(148500, 38400, &p) || p.feedbackInteger != 116)
		failures++;
	if (PlanDekelHdmiPll(0, 19200, &p)
		|| PlanDekelHdmiPll(148500, 25000, &p)
		|| PlanDekelHdmiPll(28100, 19200, &p)
		|| PlanDekelHdmiPll(0xffffffffu, 19200, &p)
		|| PlanDekelHdmiPll(148500, 19200, 0))
		failures++;
	if (failures != 0) {
		fprintf(stderr, "%u PLL checks failed\n", failures);
		return 1;
	}
	puts("8 HDMI PLL divider and input checks passed; no hardware access");
	return 0;
}
