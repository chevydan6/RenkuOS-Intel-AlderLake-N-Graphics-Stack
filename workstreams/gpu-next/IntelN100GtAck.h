// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_GT_ACK_H
#define INTEL_N100_GT_ACK_H
#include <stdint.h>

// Two raw observations only. Equal/clear bits prove neither request ownership,
// a working forcewake handshake, engine readiness nor a persistent power hold.
struct IntelN100GtAckSample {
	uint32_t first;
	uint32_t second;
};
static const uint32_t kIntelN100GtAckOffset = 0x130044;
#endif
