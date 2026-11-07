// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_RESET_DOMAINS_H
#define INTEL_N100_RESET_DOMAINS_H
#include <stdint.h>

// Offline domain planner only. Raw fuse must be sampled under owned GT wake.
// Pin/source: BCS-RESET-DESIGN.md. This is NOT a forcewake acquisition or a
// substitute for a live identity/ownership/power check in the eventual adapter.
struct IntelN100ResetDomains {
	enum Kind : uint32_t { Gt, Render, Vdbox0, Vdbox2, Vebox0 };
	struct Domain { uint32_t kind, request, ack; } domains[5];
	uint32_t count, rawFuse;
};

static inline bool
intel_n100_reset_domains(uint32_t pciIdentity, uint8_t revision,
	uint32_t mediaDisableFuse, IntelN100ResetDomains& output)
{
	if (pciIdentity != 0x46d18086 || revision != 0 || mediaDisableFuse == UINT32_MAX)
		return false;
	IntelN100ResetDomains result = {};
	result.rawFuse = mediaDisableFuse;
	result.domains[result.count++] = { IntelN100ResetDomains::Gt, 0xa188, 0x130044 };
	result.domains[result.count++] = { IntelN100ResetDomains::Render, 0xa278, 0xd84 };
	// ADL-N uses adl_p_info: only VCS0, VCS2 and VECS0 are candidates.
	// Gen12.0 fuse bits are DISABLE, unlike >=12.55 enable semantics.
	if ((mediaDisableFuse & (1u << 0)) == 0)
		result.domains[result.count++] = { IntelN100ResetDomains::Vdbox0, 0xa540, 0xd50 };
	if ((mediaDisableFuse & (1u << 2)) == 0)
		result.domains[result.count++] = { IntelN100ResetDomains::Vdbox2, 0xa548, 0xd58 };
	if ((mediaDisableFuse & (1u << 16)) == 0)
		result.domains[result.count++] = { IntelN100ResetDomains::Vebox0, 0xa560, 0xd70 };
	output = result;
	return true;
}
#endif
