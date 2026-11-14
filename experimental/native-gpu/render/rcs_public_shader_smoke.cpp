// SPDX-License-Identifier: MIT
#include "RcsPublicShader.h"
#include <stdio.h>
#include <string.h>

int main() {
 alignas(4096) uint32_t batch[1024]={},shader[1024]={};
 memcpy(batch,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 memcpy(shader,NativeGpu::RcsDrawData::Fs,sizeof(NativeGpu::RcsDrawData::Fs));
 if(!NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 1;
 if(!NativeGpu::RcsPublicShader::PatchBatch(batch,1024,0x410000))return 2;
 if(batch[44]!=0x20001 || batch[239]!=0x10000 || batch[248]!=0x10080)return 3;
 if(!NativeGpu::RcsPublicShader::ValidateBatch(batch,1024,0x410000))return 4;
 batch[0]^=1;
 if(NativeGpu::RcsPublicShader::ValidateBatch(batch,1024,0x410000))return 5;
 batch[0]^=1;
 shader[0]^=1;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 6;
 shader[0]^=1;
 shader[1023]=1;
 if(NativeGpu::RcsPublicShader::ValidateFragment(shader,1024))return 7;
 shader[1023]=0;
 uint32_t original[1024]={};
 memcpy(original,NativeGpu::RcsDrawData::Batch,sizeof(NativeGpu::RcsDrawData::Batch));
 uint32_t invalid[1024];memcpy(invalid,original,sizeof(invalid));
 if(NativeGpu::RcsPublicShader::PatchBatch(invalid,1024,0x420000)
  || memcmp(invalid,original,sizeof(invalid)))return 8;
 invalid[1]^=1;uint32_t before[1024];memcpy(before,invalid,sizeof(before));
 if(NativeGpu::RcsPublicShader::PatchBatch(invalid,1024,0x410000)
  || memcmp(invalid,before,sizeof(invalid)))return 9;
 puts("N100 public FS page and three-word bounded batch patch: PASS (CPU only)");
 return 0;
}
