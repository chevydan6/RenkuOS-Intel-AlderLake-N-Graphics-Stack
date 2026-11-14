// SPDX-License-Identifier: MIT
#include "CompiledKnownShader.h"
#include <stdio.h>
#include <string.h>

int main()
{
 uint8_t fragment[sizeof(NativeGpu::RcsDrawData::Fs)]={};
 if(!CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),fragment,sizeof(fragment)))return 1;
 if(memcmp(fragment,NativeGpu::RcsDrawData::Fs,sizeof(fragment)))return 2;
 uint32_t altered[sizeof(RenkuKnownSpirv::Fragment)/sizeof(uint32_t)];
 memcpy(altered,RenkuKnownSpirv::Fragment,sizeof(altered));
 altered[5]^=1;
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),altered,sizeof(altered),
    fragment,sizeof(fragment)))return 3;
 if(CompileKnownFragment(RenkuKnownSpirv::Vertex,
    sizeof(RenkuKnownSpirv::Vertex),RenkuKnownSpirv::Fragment,
    sizeof(RenkuKnownSpirv::Fragment),fragment,1))return 4;
 puts("compiler pipeline probe: matched hardware-tested fragment; rejected altered input and short buffer");
 return 0;
}
