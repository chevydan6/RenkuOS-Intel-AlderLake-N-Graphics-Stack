// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_RCS_PCI_ADMISSION_H
#define NATIVE_GPU_RCS_PCI_ADMISSION_H
#include <stdint.h>
namespace NativeGpu {
struct RcsPciAdmission {
 // RCS full-capture adapter only. Raw PCI values remain in diagnostics.
 static uint32_t Normalize(uint16_t offset,uint8_t size,uint32_t value) {
  // Linux v6.6 include/uapi/linux/pci_regs.h: PCI_STATUS_INTERRUPT=0x08.
  // Keep all-ones and out-of-width transport failures intact for Capture::Read.
  if(offset==6 && size==2 && value<0xffffU)return value&~uint32_t(8);
  return value;
 }
};
}
#endif
