// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_KERNEL_PAGES_H
#define INTEL_N100_KERNEL_PAGES_H
#include <KernelExport.h>
#include "IntelN100PpgttTree.h"
#include "IntelDmarReserved.h"

// Wired CPU-page provider for OFFLINE table construction, not a GPU DMA mapper.
// Embed with its tree in a resident kernel module; serialize every operation.
// The module/code and record must outlive all retained areas. No external area
// deletion, ID export or CPU alias is permitted. No GPU publication is supported.
class IntelN100KernelPages {
public:
	IntelN100KernelPages() : fSlots(), fNextToken(0), fActive(false), fError(B_OK), fReserved(), fConfigured(false) {}
	IntelN100PpgttTree::Backend Backend()
		{ return {this, Retain, Drop, Allocate, Free}; }
	bool ConfigureReserved(const uint8_t* bytes,size_t size)
	{
		if(fActive||LiveAreas()!=0)return false;
		IntelDmarReserved candidate;
		if(!intel_dmar_reserved(bytes,size,&candidate))return false;
		fReserved=candidate;fConfigured=true;return true;
	}
	unsigned ReservedRanges() const { return fReserved.count; }
	status_t LastError() const { return fError; }
	unsigned LiveAreas() const
	{
		unsigned count = 0;
		for (const Slot& slot : fSlots) if (slot.token) count++;
		return count;
	}
private:
	struct Slot { uint64_t token; area_id area; bool failedDelete; };
	IntelN100KernelPages(const IntelN100KernelPages&) = delete;
	IntelN100KernelPages& operator=(const IntelN100KernelPages&) = delete;
	static bool Retain(void* context)
	{
		IntelN100KernelPages& self = *(IntelN100KernelPages*)context;
		if (!self.fConfigured || B_PAGE_SIZE != 4096 || self.fActive || self.LiveAreas() != 0)
			return false;
		self.fActive = true;
		self.fError = B_OK;
		return true;
	}
	static void Drop(void* context)
	{
		IntelN100KernelPages& self = *(IntelN100KernelPages*)context;
		// Tree calls this only after all tokens have been released.
		if (self.LiveAreas() == 0) self.fActive = false;
	}
	static bool Allocate(void* context, IntelN100PpgttTree::Page& output)
	{
		IntelN100KernelPages& self = *(IntelN100KernelPages*)context;
		if (!self.fActive || self.fNextToken == UINT64_MAX) return false;
		Slot* slot = nullptr;
		for (Slot& candidate : self.fSlots) {
			if (!candidate.token) { slot = &candidate; break; }
		}
		if (!slot) { self.fError = B_NO_MEMORY; return false; }
		void* cpu = nullptr;
		area_id area = create_area("intel private table page", &cpu,
			B_ANY_KERNEL_ADDRESS, 4096, B_FULL_LOCK,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (area < B_OK) { self.fError = area; return false; }
		*slot = {++self.fNextToken, area, false}; // Record before validation.
		output = {slot->token, (uint64_t*)cpu, UINT64_MAX};
		uintptr_t address = reinterpret_cast<uintptr_t>(cpu);
		if (!address || (address & 4095) || address > UINTPTR_MAX - 4095) {
			self.fError = B_BAD_ADDRESS;
			return true; // Acquired; tree retains invalid descriptor for diagnosis.
		}
		physical_entry entry = {};
		uint32 count = 1;
		status_t status = get_memory_map_etc(B_CURRENT_TEAM, cpu, 4096, &entry, &count);
		if (status != B_OK || count != 1 || entry.size < 4096) {
			self.fError = status == B_OK ? B_BAD_ADDRESS : status;
			return true;
		}
		// CPU physical only. A future DMA adapter must establish translation
		// and IOMMU policy before this value can be published to the GPU.
		if (!intel_n100_ppgtt_page(entry.address)) {
			self.fError=B_BAD_ADDRESS;return true;
		}
		if (!intel_dmar_page_allowed(self.fReserved,entry.address)) {
			self.fError=B_NOT_ALLOWED;
			return true; // Keep invalid sentinel; tree retains all pages until reboot.
		}
		output.dma = entry.address;
		return true;
	}
	static bool Free(void* context, uint64_t token)
	{
		IntelN100KernelPages& self = *(IntelN100KernelPages*)context;
		if (!self.fActive || !token) return false;
		for (Slot& slot : self.fSlots) {
			if (slot.token != token) continue;
			if (slot.failedDelete) return false;
			status_t status = delete_area(slot.area);
			if (status != B_OK) {
				self.fError = status;
				// Do not retry an unexpected lookup failure against a possibly
				// recycled area ID. Retain the record and context until reboot.
				slot.failedDelete = true;
				return false;
			}
			slot = {};
			return true;
		}
		return false;
	}
	Slot fSlots[24];
	uint64_t fNextToken;
	bool fActive;
	status_t fError;
	IntelDmarReserved fReserved;
	bool fConfigured;
};
#endif
