// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_COMPLETION_OBSERVATION_H
#define NATIVE_GPU_RCS_COMPLETION_OBSERVATION_H
#include "../RenderReport.h"
namespace NativeGpu {
// Administrative observation check only: request commands and actual HWSP are
// unchanged. A hidden tail prevents proving retirement, even if work finished.
struct RcsCompletionObservation {
 static bool ConsumeTimeout(bool& armed,uint32_t mode,bool accepted) {
  if(!accepted || mode!=NATIVE_GPU_RENDER_DRAW)return false;
  bool hide=armed;armed=false;return hide;
 }
 static uint32_t Tail(bool hidden,uint32_t actual,uint32_t initial) {
  return hidden?initial:uint8_t(actual);
 }
};
}
#endif
