// SPDX-License-Identifier: MIT
#include "IntelN100BcsInventory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct Reader { unsigned count; int fail, ones; };
static bool Read(void* context, uint32_t offset, uint32_t& value)
{
	Reader& r = *(Reader*)context;
	const uint32_t expected[] = {0x22030, 0x22034, 0x2203c, 0x2209c, 0x800c};
	assert(r.count < 5 && offset == expected[r.count]);
	int call = r.count++;
	value = call == r.ones ? UINT32_MAX : offset;
	return call != r.fail;
}
int main()
{
	unsigned cases = 0;
	for (int fail = -1; fail < 5; fail++) {
		for (int ones = -1; ones < 5; ones++) {
			Reader reader = {0, fail, ones};
			IntelN100BcsInventory sample, saved;
			memset(&sample, 0xa5, sizeof(sample)); saved = sample;
			bool success = intel_n100_sample_bcs(Read, &reader, sample);
			assert(success == (fail == -1 && ones == -1));
			if (!success) assert(memcmp(&sample, &saved, sizeof(sample)) == 0);
			else assert(sample.tail == 0x22030 && sample.head == 0x22034
				&& sample.control == 0x2203c && sample.mode == 0x2209c && sample.pendingWake == 0x800c);
			cases++;
		}
	}
	IntelN100BcsInventory sample = {};
	assert(!intel_n100_sample_bcs(NULL, NULL, sample)); cases++;
	printf("%u BCS inventory cases passed; mocked register reads only.\n", cases);
}
