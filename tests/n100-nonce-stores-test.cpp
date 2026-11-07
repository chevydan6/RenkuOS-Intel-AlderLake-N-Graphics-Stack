// SPDX-License-Identifier: MIT
#include "IntelN100NonceStores.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main()
{
	unsigned cases = 0;
	uint32_t guarded[10];
	for (unsigned i = 0; i < 10; i++) guarded[i] = 0xfeed1234;
	uint32_t* words = guarded + 1;
	const uint32_t golden[] = {0x10400002,0x00123000,0,0x12345678,
		0x10400002,0x00123004,0,0xedcba987};
	assert(intel_n100_ggtt_nonce_stores(0x00123000,0x12345678,words,8));
	assert(memcmp(words,golden,sizeof(golden))==0); cases++;
	const uint64_t pages[] = {0,4096,0x12345000,0xfffff000};
	const uint32_t nonces[] = {0,1,0x12345678,0xffffffff};
	for(uint64_t page : pages) {
		for(uint32_t nonce : nonces) {
			assert(intel_n100_ggtt_nonce_stores(page,nonce,words,8));
			assert(words[0]==0x10400002 && words[4]==0x10400002);
			assert(words[1]==page && words[5]==page+4 && words[2]==0 && words[6]==0);
			assert(words[3]==nonce && (words[3]^words[7])==0xffffffff);
			assert(guarded[0]==0xfeed1234 && guarded[9]==0xfeed1234); cases++;
		}
	}
	uint32_t saved[10]; memcpy(saved,guarded,sizeof(saved));
	const uint64_t invalid[] = {1,4095,4097,0xfffff004,1ULL<<32,UINT64_MAX};
	for(uint64_t page : invalid) {
		assert(!intel_n100_ggtt_nonce_stores(page,1,words,8));
		assert(memcmp(saved,guarded,sizeof(saved))==0); cases++;
	}
	for(unsigned capacity=0;capacity<8;capacity++) {
		assert(!intel_n100_ggtt_nonce_stores(4096,1,words,capacity));
		assert(memcmp(saved,guarded,sizeof(saved))==0); cases++;
	}
	assert(!intel_n100_ggtt_nonce_stores(4096,1,NULL,8)); cases++;
	printf("%u nonce packet cases passed; offline encoding only.\n",cases);
}
