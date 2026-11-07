// SPDX-License-Identifier: MIT
#include "IntelVtdObservation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
struct Reader {unsigned calls;int fail;uint32_t status;};
static bool Read(void* context,uint32_t offset,uint32_t& value)
{
	Reader& reader=*(Reader*)context;
	const uint32_t expected[]={0x1c,0,8,12,16,20,0,0x1c};
	assert(reader.calls<8&&offset==expected[reader.calls]);
	if(int(reader.calls++)==reader.fail)return false;
	value=offset==0?0x10:offset==0x1c?reader.status:offset==8?0x123:0;
	return true;
}
int main()
{
	unsigned cases=0;
	for(unsigned bit=0;bit<31;bit++) {
		for(unsigned enabled=0;enabled<2;enabled++) {
			Reader reader={0,-1,(1u<<bit)|(enabled<<31)};
			IntelVtdSample sample={};
			assert(intel_vtd_sample(Read,&reader,sample)&&reader.calls==8);
			assert(intel_vtd_translation(sample)==(enabled?IntelVtdEnabled:IntelVtdDisabled));cases++;
		}
	}
	for(int fail=0;fail<8;fail++) {
		Reader reader={0,fail,0};IntelVtdSample sample,saved;
		memset(&sample,0xa5,sizeof(sample));saved=sample;
		assert(!intel_vtd_sample(Read,&reader,sample)&&reader.calls==unsigned(fail+1));
		assert(memcmp(&sample,&saved,sizeof(sample))==0);cases++;
	}
	IntelVtdSample good={0x10,0x10,0,0,0x123,0};
	for(unsigned invalid=0;invalid<7;invalid++) {
		IntelVtdSample sample=good;
		switch(invalid) {
			case 0:sample.versionBefore=0;break;
			case 1:sample.versionBefore=UINT32_MAX;break;
			case 2:sample.versionAfter++;break;
			case 3:sample.statusAfter++;break;
			case 4:sample.capability=0;break;
			case 5:sample.capability=UINT64_MAX;break;
			case 6:sample.extendedCapability=UINT64_MAX;break;
		}
		assert(intel_vtd_translation(sample)==IntelVtdUnknown);cases++;
	}
	good.statusBefore=good.statusAfter=UINT32_MAX;
	assert(intel_vtd_translation(good)==IntelVtdUnknown);cases++;
	assert(!intel_vtd_sample(nullptr,nullptr,good));cases++;
	printf("%u VT-d observation cases passed; mocked reads only.\n",cases);
}
