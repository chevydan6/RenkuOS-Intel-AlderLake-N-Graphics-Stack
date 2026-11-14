// SPDX-License-Identifier: MIT
#include "OwnedBuffers.h"
#include <assert.h>
#include <stdio.h>
int main()
{
 NativeGpu::OwnedBuffers b={};
 assert(b.Create(4096)==1); assert(b.Create(4096)==2); assert(b.Create(4096)==0);
 native_gpu_copy c={1,2,17,5,80,96,{}};
 assert(b.CanCopy(c));
 c.destination=1;assert(!b.CanCopy(c));c.destination=2;
 c.width=0;assert(!b.CanCopy(c));c.width=17;
 c.source_pitch=64;assert(!b.CanCopy(c));c.source_pitch=80;
 c.height=100;assert(!b.CanCopy(c));c.height=5;
 c.destination_pitch=95;assert(!b.CanCopy(c));c.destination_pitch=96;
 assert(!b.Range(3,0,4));assert(!b.Range(1,4090,16));
 assert(b.Begin(c));assert(!b.CanCopy(c));assert(!b.Range(1,0,4));
 b.Finish(false,false);assert(b.CanCopy(c));assert(b.Begin(c));
 b.Finish(true);assert(b.Range(2,0,4096));assert(b.Begin(c));
 b.Finish(false);assert(!b.CanCopy(c));assert(!b.Range(2,0,4));
 NativeGpu::OwnedBuffers other={};assert(!other.CanCopy(c));
 puts("Owned buffer/copy admission and lifetime: PASS");
}
