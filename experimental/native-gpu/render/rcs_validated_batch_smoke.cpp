// SPDX-License-Identifier: MIT
#include "RcsValidatedBatch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using NativeGpu::RcsValidatedBatch;
int main() {
 uint32_t words[1024]={};
 memcpy(words,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 assert(RcsValidatedBatch::PatchVertex(words,0x411000));
 assert(RcsValidatedBatch::Validate(words,1024,0x411000));
 assert(!RcsValidatedBatch::Validate(words,1024,0x412000));
 assert(!RcsValidatedBatch::Validate(words,1023,0x411000));
 words[0]^=1;assert(!RcsValidatedBatch::Validate(words,1024,0x411000));words[0]^=1;
 words[1023]=1;assert(!RcsValidatedBatch::Validate(words,1024,0x411000));words[1023]=0;
 assert(!RcsValidatedBatch::PatchVertex(words,0x410001));
 assert(!RcsValidatedBatch::PatchVertex(words,0x420000));
 puts("N100 exact public RCS batch validation: PASS (CPU only)");
}
