/* SPDX-License-Identifier: MIT */
#include "AlderLakeCloneMmio.h"
#include <vector>
#include <stdio.h>
int main(){unsigned errors=0;std::vector<uint32> words(0x165000/4,0);uint32 value;
 AlderLakeCloneMmio m((volatile uint8*)words.data(),words.size()*4);auto io=m.Ops();
 if(io.write(io.cookie,0x71180,0x94000008)!=B_BAD_VALUE)errors++;
 m.SetPoweredPipes(7);
 if(io.read(io.cookie,0x70180,&value)!=B_OK||io.write(io.cookie,0x70180,0)!=B_BAD_VALUE
  ||io.write(io.cookie,0x7027c,0)!=B_BAD_VALUE||io.write(io.cookie,0x68180,0)!=B_BAD_VALUE)errors++;
 if(io.write(io.cookie,0x71180,0x94000008)!=B_OK||io.write(io.cookie,0x68980,0)!=B_OK
  ||io.write(io.cookie,0x7127c,0x0fff0800)!=B_OK)errors++;
 if(io.write(io.cookie,0x4438c,0)!=B_BAD_VALUE||io.write(io.cookie,0x45404,0)!=B_BAD_VALUE
  ||io.read(io.cookie,0x73008,&value)!=B_BAD_VALUE)errors++;
 AlderLakeCloneMmio small((volatile uint8*)words.data(),0x1000);small.SetPoweredPipes(7);auto no=small.Ops();
 if(no.write(no.cookie,0x71180,0)!=B_BAD_VALUE||no.read(no.cookie,0x70180,&value)!=B_BAD_VALUE)errors++;
 if(errors){fprintf(stderr,"%u clone MMIO access failures\n",errors);return 1;}
 puts("5 clone MMIO bounds, powered-pipe and source-write isolation cases passed");return 0;}
