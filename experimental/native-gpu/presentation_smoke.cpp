// SPDX-License-Identifier: MIT
#include "PresentationFrame.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main()
{
 uint8_t src[24],dst[32];memset(src,0x36,sizeof(src));memset(dst,0xa5,sizeof(dst));
 NativeGpu::FrameDescription shape={2,2,12,NativeGpu::OpaqueBgrx32};
 assert(NativeGpu::CopyFrameRows(shape,src,sizeof(src),dst,sizeof(dst),16));
 for(unsigned i=0;i<32;i++)assert(dst[i]==((i<8 || (i>=16 && i<24))?0x36:0xa5));
 shape.format=99;assert(!NativeGpu::CopyFrameRows(shape,src,sizeof(src),dst,sizeof(dst),16));
 shape.format=NativeGpu::OpaqueBgrx32;shape.stride=4;
 assert(!NativeGpu::CopyFrameRows(shape,src,sizeof(src),dst,sizeof(dst),16));
 NativeGpu::CompletionGate gate;
 native_gpu_execution_report r={};r.version=1;r.status=0;r.stage=14;r.load_attempted=1;
 r.sequence=r.observed_sequence=5;r.nonce=r.observed_nonce=0x1234;r.observed_complement=~r.nonce;
 r.copy_words=r.copy_matched=1024;r.mode=0x200;
 assert(gate.Complete(r));assert(gate.LastSequence()==5);
 assert(!gate.Complete(r));assert(!gate.CanSubmit());
 r.sequence=r.observed_sequence=6;assert(!gate.Complete(r));
 NativeGpu::CompletionGate failed;failed.Fail();assert(!failed.Complete(r));
 puts("Presentation stride/format and completion/fail-stop policy: PASS");
}
