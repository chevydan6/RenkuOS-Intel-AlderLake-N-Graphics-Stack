// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
struct IntelVtdSample {
	uint32_t versionBefore, versionAfter, statusBefore, statusAfter;
	uint64_t capability, extendedCapability;
};
enum IntelVtdTranslation { IntelVtdUnknown, IntelVtdDisabled, IntelVtdEnabled };
// Read-only register observations per Intel VT-d Rev4.1 sections11.2/11.4.
// No copying of Linux IOMMU implementation. Callback performs aligned UC32 reads.
static inline bool
intel_vtd_sample(bool (*read)(void*,uint32_t,uint32_t&),void* context,IntelVtdSample& output)
{
	if(!read) return false;
	IntelVtdSample sample={};
	uint32_t capLow=0,capHigh=0,extLow=0,extHigh=0;
	if(!read(context,0x1c,sample.statusBefore)||!read(context,0,sample.versionBefore)
		||!read(context,8,capLow)||!read(context,12,capHigh)
		||!read(context,16,extLow)||!read(context,20,extHigh)
		||!read(context,0,sample.versionAfter)||!read(context,0x1c,sample.statusAfter))
		return false;
	sample.capability=uint64_t(capLow)|(uint64_t(capHigh)<<32);
	sample.extendedCapability=uint64_t(extLow)|(uint64_t(extHigh)<<32);
	output=sample;
	return true;
}
static inline IntelVtdTranslation intel_vtd_translation(const IntelVtdSample& sample)
{
	if(!sample.versionBefore||sample.versionBefore==UINT32_MAX
		||sample.versionBefore!=sample.versionAfter||!sample.capability
		||sample.capability==UINT64_MAX||sample.extendedCapability==UINT64_MAX
		||sample.statusBefore==UINT32_MAX||sample.statusBefore!=sample.statusAfter)
		return IntelVtdUnknown;
	return (sample.statusBefore&(1u<<31))?IntelVtdEnabled:IntelVtdDisabled;
}
