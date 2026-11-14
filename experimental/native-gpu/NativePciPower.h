// SPDX-License-Identifier: MIT
#ifndef NATIVE_GPU_PCI_POWER_H
#define NATIVE_GPU_PCI_POWER_H
#include <IntelN100PciPower.h>
// Initial admission still uses the complete native PCI capability capture.
// Runtime checks validate access/power anchors. PCI Status contains volatile
// interrupt/error bits, and PMCSR has PME status; neither is an immutable device
// identity. A complete capability walk around every ACK load adds unrelated
// snapshot-equality constraints to the MMIO access contract.
inline bool native_gpu_pci_power(IntelN100PciPower::Reader read,void* context,
	const IntelN100PciSnapshot& expected,IntelN100PciSnapshot& observed)
{
	if(!read || expected.identity!=0x46d18086 || expected.classRevision!=0x03000000
		|| (expected.command&6)!=6 || expected.pmOffset<0x40 || expected.pmOffset>0xf8)
		return false;
	observed=expected;
	for(unsigned pass=0;pass<2;pass++) {
		uint32_t command=0,status=0,power=0;
		if(!read(context,0,4,observed.identity) || !read(context,8,4,observed.classRevision)
			|| !read(context,0x10,4,observed.barLow) || !read(context,0x14,4,observed.barHigh)
			|| !read(context,4,2,command) || !read(context,6,2,status)
			|| !read(context,expected.pmOffset+4,2,power))return false;
		observed.command=command;observed.status=status;observed.pmControl=power;
		if(observed.identity!=expected.identity || observed.classRevision!=expected.classRevision
			|| observed.barLow!=expected.barLow || observed.barHigh!=expected.barHigh
			|| command!=expected.command || status>=0xffff || !(status&0x10)
			|| power>=0xffff || (power&3)!=0)return false;
	}
	return true;
}
#endif
