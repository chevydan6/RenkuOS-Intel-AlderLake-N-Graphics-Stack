// SPDX-License-Identifier: MIT
#include "IntelN100ResetDomains.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
	unsigned cases = 0;
	// Exhaust every documented VDBOX/VEBOX fuse-field combination, including
	// bits for engines absent from the platform's initial engine mask.
	for (uint32_t vdbox = 0; vdbox < 256; vdbox++) {
		for (uint32_t vebox = 0; vebox < 16; vebox++) {
			IntelN100ResetDomains result;
			uint32_t fuse = vdbox | (vebox << 16);
			assert(intel_n100_reset_domains(0x46d18086, 0, fuse, result));
			assert(result.rawFuse == fuse && result.count >= 2 && result.count <= 5);
			unsigned seen = 0;
			for (unsigned n = 0; n < result.count; n++) {
				const auto& d = result.domains[n];
				assert(d.kind < 5 && !(seen & (1u << d.kind)));
				seen |= 1u << d.kind;
				const uint32_t requests[] = {0xa188, 0xa278, 0xa540, 0xa548, 0xa560};
				const uint32_t acks[] = {0x130044, 0xd84, 0xd50, 0xd58, 0xd70};
				assert(d.request == requests[d.kind] && d.ack == acks[d.kind]);
			}
			assert((seen & 3) == 3);
			assert(bool(seen & 4) == (vdbox % 2 == 0));
			assert(bool(seen & 8) == ((vdbox / 4) % 2 == 0));
			assert(bool(seen & 16) == (vebox % 2 == 0));
			cases++;
		}
	}
	for (unsigned fault = 0; fault < 5; fault++) {
		IntelN100ResetDomains output, before;
		memset(&output, 0xa5, sizeof(output)); before = output;
		uint32_t id = 0x46d18086, fuse = 0; uint8_t revision = 0;
		if (fault == 0) id = 0x46d18087;
		if (fault == 1) id = 0x46d08086;
		if (fault == 2) revision = 1;
		if (fault == 3) revision = 255;
		if (fault == 4) fuse = UINT32_MAX;
		assert(!intel_n100_reset_domains(id, revision, fuse, output));
		assert(memcmp(&output, &before, sizeof(output)) == 0); cases++;
	}
	printf("%u reset-domain planning cases passed; no MMIO or forcewake writes.\n", cases);
}
