/* SPDX-License-Identifier: MIT */
#include "DisplayPipeLease.h"
#include <stdio.h>
int main() {
 unsigned errors=0;uint32 mask=0;
 DisplayPipeLease a(&mask,1),b(&mask,1),c(&mask,2),bad(&mask,3),nil(NULL,0);
 if(a.Acquire()!=B_OK||mask!=2||b.Acquire()!=B_BUSY||b.Release()!=B_OK||mask!=2)errors++;
 if(c.Acquire()!=B_OK||mask!=6||a.Acquire()!=B_BUSY)errors++;
 if(a.Release()!=B_OK||mask!=4||b.Acquire()!=B_OK||mask!=6)errors++;
 if(c.Release()!=B_OK||mask!=2||b.Release()!=B_OK||mask)errors++;
 if(bad.Acquire()!=B_BAD_VALUE||nil.Acquire()!=B_BAD_VALUE||mask)errors++;
 if(a.Acquire()!=B_OK)errors++;
 mask=0;
 if(a.Release()!=B_ERROR||!a.IsHeld())errors++;
 mask=2;if(a.Release()!=B_OK||a.IsHeld()||mask)errors++;
 if(errors){fprintf(stderr,"%u pipe lease failures\n",errors);return 1;}
 puts("8 shared display pipe ownership and recovery cases passed");return 0;
}
