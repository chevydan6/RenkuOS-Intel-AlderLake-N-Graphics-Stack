// SPDX-License-Identifier: MIT
#include "IntelDmarInventory.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
static void Seal(uint8_t* b,size_t size) {
	b[4]=size&255;b[5]=(size>>8)&255;b[6]=b[7]=0;b[9]=0;
	uint8_t sum=0;for(size_t i=0;i<size;i++)sum+=b[i];b[9]=-sum;
}
int main() {
	uint8_t bytes[4096]={}; memcpy(bytes,"DMAR",4);
	bytes[50]=24;bytes[52]=1;bytes[56]=0;bytes[57]=0x10; // include-all,base0x1000
	bytes[64]=1;bytes[65]=8;bytes[69]=0;bytes[70]=2;bytes[71]=0;
	Seal(bytes,72);
	IntelDmarInventory result={};
	assert(intel_dmar_inventory(bytes,72,&result)&&result.unitCount==1);
	assert(result.units[0].registers==4096&&result.units[0].scopes==1&&result.units[0].flags==1);
	unsigned cases=1;
	IntelDmarInventory saved=result;
	for(unsigned i=0;i<72;i++) {
		bytes[i]^=1;assert(!intel_dmar_inventory(bytes,72,&result));
		assert(memcmp(&result,&saved,sizeof(result))==0);bytes[i]^=1;cases++;
	}
	const unsigned positions[]={50,65,70,71};
	const unsigned values[]={0,7,32,8};
	for(unsigned i=0;i<4;i++) {
		uint8_t old=bytes[positions[i]];bytes[positions[i]]=values[i];Seal(bytes,72);
		assert(!intel_dmar_inventory(bytes,72,&result));bytes[positions[i]]=old;Seal(bytes,72);cases++;
	}
	bytes[64]=99;Seal(bytes,72);
	assert(intel_dmar_inventory(bytes,72,&result)&&result.unknownScopes==1);cases++;
	bytes[48]=99;Seal(bytes,72);
	assert(intel_dmar_inventory(bytes,72,&result)&&result.unitCount==0&&result.unknownRecords==1);cases++;
	assert(!intel_dmar_inventory(nullptr,72,&result));
	assert(!intel_dmar_inventory(bytes,72,nullptr));
	assert(!intel_dmar_inventory(bytes,47,&result));
	assert(!intel_dmar_inventory(bytes,4097,&result));cases+=4;

	memset(bytes,0,sizeof(bytes));memcpy(bytes,"DMAR",4);
	for(unsigned i=0;i<17;i++) bytes[48+i*16+2]=16;
	Seal(bytes,48+16*16);
	assert(intel_dmar_inventory(bytes,48+16*16,&result)&&result.unitCount==16);cases++;
	saved=result;
	Seal(bytes,48+17*16);
	assert(!intel_dmar_inventory(bytes,48+17*16,&result));
	assert(memcmp(&result,&saved,sizeof(result))==0);cases++;
	const unsigned truncated[]={49,50,51,63};
	for(unsigned length:truncated) {
		Seal(bytes,length);assert(!intel_dmar_inventory(bytes,length,&result));cases++;
	}
	memset(bytes,0,sizeof(bytes));memcpy(bytes,"DMAR",4);
	bytes[48]=99;bytes[50]=0xd0;bytes[51]=0x0f;Seal(bytes,4096);
	assert(intel_dmar_inventory(bytes,4096,&result)&&result.unknownRecords==1);cases++;
	memset(bytes,0,sizeof(bytes));memcpy(bytes,"DMAR",4);
	bytes[50]=16;bytes[64]=99;bytes[66]=4;bytes[70]=16;Seal(bytes,84);
	assert(intel_dmar_inventory(bytes,84,&result)&&result.unitCount==2&&result.unknownRecords==1);cases++;
	memset(bytes,0,sizeof(bytes));memcpy(bytes,"DMAR",4);
	bytes[50]=24;bytes[64]=1;
	for(unsigned length: {0u,5u,6u,7u,9u}) {
		bytes[65]=length;Seal(bytes,72);
		assert(!intel_dmar_inventory(bytes,72,&result));cases++;
	}
	printf("%u DMAR inventory parser cases passed; no hardware access.\n",cases);
}
