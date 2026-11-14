// SPDX-License-Identifier: MIT
#include <IntelN100CsbReader.h>
#include <assert.h>
#include <stdio.h>
struct Captured {
	uint64_t now=0;
	static uint64_t Now(void* p){return ++((Captured*)p)->now;}
	static bool Safe(void*){return true;}
	static bool Read32(void*,uint32_t offset,uint32_t& value)
	{if(offset!=kN100CsbWrite)return false;value=2;return true;}
	static bool Read64(void*,uint32_t offset,uint64_t& value)
	{
		// Exact GPU-written records captured by native execution attempt004.
		if(offset==0x48)value=0x03ff800000008019ULL;
		else if(offset==0x50)value=0x0000800003ff8018ULL;
		else value=UINT64_MAX;
		return true;
	}
	static void Relax(void*){}
};
int main()
{
	Captured captured;
	IntelN100CsbReader reader;
	IntelN100CsbReader::Io io={&captured,Captured::Now,Captured::Safe,
		Captured::Read32,Captured::Read64,Captured::Safe,Captured::Relax};
	assert(reader.Run(io,1,1,10000,8,0));
	assert(reader.LastReport().events.accepted==2);
	assert(reader.LastReport().events.state==IntelN100ContextStatus::SwitchedAway);
	puts("Captured N100 CSB cursor0 / slots1-2: PASS");
}
