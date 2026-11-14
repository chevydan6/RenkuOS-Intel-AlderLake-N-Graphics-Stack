// SPDX-License-Identifier: MIT
#include "NativePciPower.h"
#include <assert.h>
#include <stdio.h>
struct Device { bool moved=false; };
static bool Read(void* p,uint16_t offset,uint8_t,uint32_t& v)
{
	auto& d=*(Device*)p;
	switch(offset){case 0:v=0x46d18086;break;case 8:v=0x03000000;break;
	case 0x10:v=d.moved?0x200004:0x100004;break;case 0x14:v=0;break;
	case 4:v=0x407;break;case 6:v=0x18;break;case 0x54:v=0x8000;break;
	default:return false;}return true;
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
	puts("Native PCI access anchors: PASS (volatile status accepted; moved BAR rejected)");
}
