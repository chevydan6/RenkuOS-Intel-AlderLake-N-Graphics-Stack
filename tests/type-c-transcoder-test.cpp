/* SPDX-License-Identifier: MIT */
#include "TypeCHdmiTranscoder.h"
#include <stdio.h>

int main() {
	unsigned errors = 0;
	const unsigned bpc[] = {6, 8, 10, 12};
	const unsigned encoded[] = {2, 0, 1, 3};
	for (unsigned selection = 1; selection <= 9; selection++) {
		for (unsigned i = 0; i < 4; i++) {
			for (unsigned sync = 0; sync < 4; sync++) {
				TypeCHdmiTranscoderPlan p = {};
				uint32 expected = (selection << 27) | (encoded[i] << 20)
					| ((sync & 1) ? (1U << 16) : 0)
					| ((sync & 2) ? (1U << 17) : 0);
				if (BuildTypeCHdmiTranscoderPlan(selection, 5, bpc[i],
						sync & 1, sync & 2, &p) != B_OK
					|| p.clockSelect != 0x50000000U
					|| p.functionDisabled != expected
					|| p.functionEnabled != (expected | 0x80000000U)) errors++;
			}
		}
	}
	TypeCHdmiTranscoderPlan p = {};
	if (BuildTypeCHdmiTranscoderPlan(0, 5, 8, false, false, &p) != B_BAD_VALUE
		|| BuildTypeCHdmiTranscoderPlan(10, 5, 8, false, false, &p) != B_BAD_VALUE
		|| BuildTypeCHdmiTranscoderPlan(5, 0, 8, false, false, &p) != B_BAD_VALUE
		|| BuildTypeCHdmiTranscoderPlan(5, 16, 8, false, false, &p) != B_BAD_VALUE
		|| BuildTypeCHdmiTranscoderPlan(5, 5, 7, false, false, &p) != B_BAD_VALUE
		|| BuildTypeCHdmiTranscoderPlan(5, 5, 8, false, false, NULL) != B_BAD_VALUE)
		errors++;
	for (unsigned tc = 1; tc <= 4; tc++) {
		if (BuildAlderLakeMobileTypeCHdmiPlan(tc, 8, false, false, &p) != B_OK
			|| p.clockSelect != (tc + 5) << 28
			|| p.functionEnabled != ((tc + 3) << 27 | 0x80000000U)) errors++;
	}
	if (BuildAlderLakeMobileTypeCHdmiPlan(0, 8, false, false, &p) != B_BAD_VALUE
		|| BuildAlderLakeMobileTypeCHdmiPlan(5, 8, false, false, &p) != B_BAD_VALUE)
		errors++;
	if (errors) { fprintf(stderr, "%u transcoder cases failed\n", errors); return 1; }
	puts("156 HDMI transcoder plan cases passed; no hardware access");
	return 0;
}
