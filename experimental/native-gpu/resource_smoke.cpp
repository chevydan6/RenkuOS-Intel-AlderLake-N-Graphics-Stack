// SPDX-License-Identifier: MIT
#include "NativeResource.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
 using Table=NativeGpu::ResourceTable;
 Table first(0x800000,0x200000),other(0x800000,0x200000);
 uint32_t a=0,b=0;uint64_t actual=0,address=0;
 assert(first.Create(1,a,actual) && a && actual==4096);
 assert(!other.Range(a,0,1) && !other.Bind(a,0x800000,0,4096));
 assert(first.Create(8192,b,actual) && b!=a && actual==8192);
 assert(!first.Create(0,b,actual));
 assert(!first.Create(UINT64_MAX,b,actual));
 assert(!first.Bind(a,0x800001,0,4096));
 assert(!first.Bind(a,0x800000,1,4096));
 assert(!first.Bind(a,0x800000,0,8192));
 assert(!first.Bind(a,0x9ff000,0,8192));
 assert(first.Bind(a,0x800000,0,4096));
 assert(!first.Bind(b,0x800000,0,4096));
 assert(first.Bind(b,0x801000,0,8192));
 assert(first.Address(a,64,4,address) && address==0x800040);
 assert(!first.Address(a,4095,2,address));
 assert(!first.Address(a,UINT64_MAX,2,address));
 assert(!first.Range(a^128,0,1)); // Forged generation.
 assert(first.Publish());
 assert(!first.Create(4096,b,actual));
 assert(!first.Bind(a,0x804000,0,4096));
 assert(first.Destroy(a));
 assert(!first.Range(a,0,1) && !first.Address(a,0,1,address));
 assert(first.RetainedCount()==1);
 assert(!first.RetireEpoch(false) && first.RetainedCount()==1);
 assert(first.RetireEpoch(true) && first.RetainedCount()==0);
 uint32_t newer=0;
 assert(first.Create(4096,newer,actual) && newer!=a);
 assert(!first.Range(a,0,1) && first.Range(newer,0,4096));
 assert(first.Destroy(newer));
 puts("Native GPU resource handles, VA bounds, and retirement: PASS (CPU only)");
}
