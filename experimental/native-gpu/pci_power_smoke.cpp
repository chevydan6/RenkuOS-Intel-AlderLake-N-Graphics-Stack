// SPDX-License-Identifier: MIT
#include "NativePciPower.h"
#include "RenderReport.h"
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#define CHECK_PCI_RESULT(publicName,privateName) static_assert(uint32_t(publicName)==uint32_t(IntelN100PciPower::privateName),"render PCI result ABI")
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_SUCCESS,Success);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_INVALID_MAPPING,InvalidMapping);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_READ_FAILED,ReadFailed);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_UNSUPPORTED_DEVICE,UnsupportedDevice);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_MEMORY_DISABLED,MemoryDisabled);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_INVALID_BAR,InvalidBar);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_INVALID_CAPABILITIES,InvalidCapabilities);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_MISSING_POWER,MissingPower);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_NOT_D0,NotD0);
CHECK_PCI_RESULT(NATIVE_GPU_RENDER_PCI_UNSTABLE,Unstable);
#undef CHECK_PCI_RESULT
static_assert(offsetof(native_gpu_render_report,pci_error)==236,"render PCI report offset");
struct Device { bool moved=false; };
static bool Read(void* p,uint16_t offset,uint8_t,uint32_t& v)
{
	auto& d=*(Device*)p;
	switch(offset){case 0:v=0x46d18086;break;case 8:v=0x03000000;break;
	case 0x10:v=d.moved?0x200004:0x100004;break;case 0x14:v=0;break;
	case 4:v=0x407;break;case 6:v=0x18;break;case 0x54:v=0x8000;break;
	default:return false;}return true;
}
struct CaptureFixture {unsigned statusReads=0;bool volatileStatus=false;};
static bool CaptureRead(void* p,uint16_t offset,uint8_t,uint32_t& value)
{
 auto& f=*(CaptureFixture*)p;
 switch(offset){
  case 0:value=0x46d18086;break;case 8:value=0x03000000;break;
  case 0x0e:value=0;break;case 4:value=0x407;break;
  case 6:value=f.volatileStatus && ++f.statusReads>1?0x18:0x10;break;
  case 0x10:value=0x100004;break;case 0x14:value=0;break;
  case 0x34:value=0x50;break;case 0x50:value=1;break;
  case 0x52:case 0x54:value=0;break;default:return false;
 }
 return true;
}
int main()
{
	IntelN100PciSnapshot expected={},observed={};
	expected.identity=0x46d18086;expected.classRevision=0x03000000;
	expected.barLow=0x100004;expected.command=0x407;expected.status=0x10;expected.pmOffset=0x50;
	Device device;
	assert(native_gpu_pci_power(Read,&device,expected,observed));
	device.moved=true;
	assert(!native_gpu_pci_power(Read,&device,expected,observed));
 CaptureFixture stable;IntelN100PciSnapshot snapshot={};
 native_gpu_render_report report={};
 report.pci_error=IntelN100PciPower::Capture(CaptureRead,&stable,0x100000,4096,snapshot);
 assert(report.pci_error==NATIVE_GPU_RENDER_PCI_SUCCESS && snapshot.identity==0x46d18086);
 CaptureFixture changing;changing.volatileStatus=true;snapshot={};
 report.pci_error=IntelN100PciPower::Capture(CaptureRead,&changing,0x100000,4096,snapshot);
 assert(report.pci_error==NATIVE_GPU_RENDER_PCI_UNSTABLE && snapshot.identity==0);
	puts("Native PCI access anchors: PASS (volatile status accepted; moved BAR rejected)");
}
