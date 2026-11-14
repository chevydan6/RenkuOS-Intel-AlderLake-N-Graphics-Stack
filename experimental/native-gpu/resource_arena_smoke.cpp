// SPDX-License-Identifier: MIT
#include "NativeResourceArena.h"
#include <assert.h>
#include <stdio.h>
using NativeGpu::ResourceArena;
int main() {
 ResourceArena arena(0x410000,16);
 uint64_t a=0,b=0;assert(arena.Open(a));assert(arena.Open(b));assert(a!=b);
 const uint64_t oldA=a;
 uint32_t x=0,y=0,z=0,w=0;uint64_t xa=0,ya=0,za=0,size=0;
 assert(!arena.Create(a,0,x,xa,size));
 assert(!arena.Create(a,65537,x,xa,size));
 assert(arena.Create(a,5000,x,xa,size));assert(size==8192 && xa==0x410000);
 assert(arena.Create(b,4096,y,ya,size));assert(ya==0x412000);
 assert(!arena.Resolve(b,x,0,1,za));
 assert(!arena.Resolve(a,x,8192,1,za));
 assert(!arena.Resolve(a,x,0,8193,za));
 assert(arena.Resolve(a,x,4095,2,za) && za==xa+4095);
 assert(arena.Resolve(b,y,0,1,za) && za==ya);
 assert(!arena.Destroy(b,x,true));
 assert(!arena.Destroy(a,x,false));
 assert(arena.Close(a,false)); // Unknown GPU use keeps the VA occupied.
 assert(!arena.Resolve(a,x,0,1,za));
 assert(arena.Create(b,4096,z,za,size) && za==0x413000);
 assert(!arena.RetireOrphans(false));
 assert(arena.RetireOrphans(true));
 assert(arena.Create(b,8192,w,za,size) && za==xa);
 assert(!arena.Resolve(oldA,x,0,1,za));
 assert(arena.Open(a) && a!=oldA); // Retired session token never revives.
 assert(arena.Destroy(b,y,true));
 assert(!arena.Resolve(b,y,0,1,za));
 assert(arena.Close(b,true));
 assert(!arena.Resolve(b,w,0,1,za));
 assert(arena.Close(a,true));
 ResourceArena invalid(0x410001,16);assert(!invalid.Open(a));
 puts("N100 prebound resource arena ownership and retirement: PASS (CPU only)");
}
