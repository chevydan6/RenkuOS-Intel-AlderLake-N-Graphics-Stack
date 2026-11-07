// SPDX-License-Identifier: MIT
#include "IntelDmarReserved.h"
#include <assert.h>
#include <stdio.h>
#include <initializer_list>
static void Store64(uint8_t* b,uint64_t value){for(unsigned i=0;i<8;i++)b[i]=value>>(8*i);}
static void Seal(uint8_t* b,unsigned n){b[4]=n&255;b[5]=n>>8;b[9]=0;uint8_t sum=0;for(unsigned i=0;i<n;i++)sum+=b[i];b[9]=-sum;}
int main()
{
	uint8_t b[4096]={};memcpy(b,"DMAR",4);
	for(unsigned i=0;i<17;i++) {
		unsigned o=48+i*32;b[o]=1;b[o+2]=32;
		Store64(b+o+8,0x7c000000);Store64(b+o+16,0x803fffff);
		b[o+24]=1;b[o+25]=8;b[o+30]=2;
	}
	unsigned cases=0;IntelDmarReserved ranges;
	for(unsigned n=0;n<=16;n++) {
		Seal(b,48+n*32);assert(intel_dmar_reserved(b,48+n*32,&ranges)&&ranges.count==n);cases++;
	}
	IntelDmarReserved saved=ranges;Seal(b,48+17*32);
	assert(!intel_dmar_reserved(b,48+17*32,&ranges)&&memcmp(&saved,&ranges,sizeof(saved))==0);cases++;
	Seal(b,80);assert(intel_dmar_reserved(b,80,&ranges));
	const uint64_t denied[]={0x7c000000,0x7c001000,0x803ff000};
	for(uint64_t page:denied){assert(!intel_dmar_page_allowed(ranges,page));cases++;}
	assert(intel_dmar_page_allowed(ranges,0x7bfff000));
	assert(intel_dmar_page_allowed(ranges,0x80400000));cases+=2;
	assert(!intel_dmar_page_allowed(ranges,1));assert(!intel_dmar_page_allowed(ranges,UINT64_MAX));cases+=2;
	Store64(b+56,UINT64_MAX-4095);Store64(b+64,UINT64_MAX);Seal(b,80);
	assert(intel_dmar_reserved(b,80,&ranges));
	assert(!intel_dmar_page_allowed(ranges,UINT64_MAX-4095));cases++;
	const uint64_t badBase[]={1,0x80400000};
	for(uint64_t base:badBase){Store64(b+56,base);Store64(b+64,0x803fffff);Seal(b,80);assert(!intel_dmar_reserved(b,80,&ranges));cases++;}
	Store64(b+56,0x7c000000);Store64(b+64,0x803ff000);Seal(b,80);
	assert(!intel_dmar_reserved(b,80,&ranges));cases++;
	Store64(b+64,0x803fffff);
	for(unsigned n: {0u,6u,7u,9u}){b[73]=n;Seal(b,80);assert(!intel_dmar_reserved(b,80,&ranges));cases++;}
	b[73]=8;b[72]=99;Seal(b,80);assert(!intel_dmar_reserved(b,80,&ranges));cases++;
	b[72]=1;b[48]=2;Seal(b,80);assert(!intel_dmar_reserved(b,80,&ranges));cases++;
	b[48]=1;Seal(b,80);b[9]++;assert(!intel_dmar_reserved(b,80,&ranges));cases++;
	printf("%u reserved-memory filter cases passed; no hardware access.\n",cases);
}
