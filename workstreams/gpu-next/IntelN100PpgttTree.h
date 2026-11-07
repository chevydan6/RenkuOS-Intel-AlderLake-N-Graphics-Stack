// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_PPGTT_TREE_H
#define INTEL_N100_PPGTT_TREE_H
#include "IntelN100Ppgtt.h"
#include <string.h>

// Offline constructor only: no GPU publication or hardware-safe cleanup API.
// All calls require external serialization, including getters and callbacks.
// Backend callbacks must not re-enter this record. The backend guarantees
// exclusive physical backing; distinct virtual aliases cannot be detected here.
// The owner must keep this record and backend code alive until Release succeeds.
// Retained/Ambiguous records must never be destroyed or overwritten. Ambiguous
// has no in-place recovery; a future resident module retains it until reboot.
class IntelN100PpgttTree {
public:
	struct Page { uint64_t token; uint64_t* cpu; uint64_t dma; };
	struct Backend {
		void* context;
		// Failure acquires nothing. Success pins context and callback lifetime.
		bool (*retain)(void*);
		void (*release)(void*); // Infallible after all pages have been released.
		// Success returns exactly one exclusively owned, CPU-writable 4K page.
		// Failure leaves output unchanged and no resource or hidden ownership.
		bool (*allocate)(void*, Page&);
		// Failure retains this exact token, without side effects preventing retry.
		bool (*free)(void*, uint64_t);
	};
	enum State { Empty, Ready, Retained, Ambiguous };
	IntelN100PpgttTree() : fBackend(), fPages(), fCount(0), fHeld(false),
		fState(Empty) {}
	State CurrentState() const { return fState; }
	unsigned RecordedPages() const { return fCount; }
	// Includes released slots (token zero) for failure diagnostics.
	bool Describe(unsigned index, Page& page) const
	{
		if (index >= fCount) return false;
		page = fPages[index];
		return true;
	}
	bool Root(uint64_t& dma) const
	{
		if (fState != Ready) return false;
		dma = fPages[0].dma;
		return true;
	}

	bool Build(const Backend& backend, uint64_t window, unsigned dataPages,
		unsigned dataPat, unsigned scratchPat, IntelN100DirectoryCache cache)
	{
		uint64_t check;
		if (fState != Empty || fHeld || fCount != 0 || dataPages == 0
			|| dataPages > 16 || (window & ((1ULL << 21) - 1)) != 0
			|| !intel_n100_ppgtt_range(window, 1ULL << 21)
			|| dataPat > 3 || scratchPat > 3
			|| !intel_n100_ppgtt_directory(0, cache, &check)
			|| !backend.retain || !backend.release || !backend.allocate || !backend.free)
			return false;
		if (!backend.retain(backend.context)) return false;
		fBackend = backend;
		fHeld = true;
		fState = Retained;
		for (unsigned i = 0; i < 8 + dataPages; i++) {
			Page& page = fPages[fCount];
			page = {};
			if (!fBackend.allocate(fBackend.context, page)) {
				// A changed failed output violates the no-hidden-ownership contract.
				if (page.token || page.cpu || page.dma) {
					fCount++;
					fState = Ambiguous;
				} else
					Release();
				return false;
			}
			fCount++; // Record before validating or touching the returned page.
			if (!ValidPage(fCount - 1)) {
				fState = Ambiguous;
				return false;
			}
		}
		// No page writes occur before the whole allocation set is validated.
		for (unsigned i = 0; i < fCount; i++)
			memset(fPages[i].cpu, 0, 4096);
		uint64_t scratchLeaf;
		intel_n100_ppgtt_leaf(fPages[7].dma, scratchPat, true, &scratchLeaf);
		// Slots0..3: active root/PDP/PD/PT. Slots4..6: scratch PDP/PD/PT.
		// Slot7: private writable scratch data; slots8+: owned diagnostic data.
		Fill(6, scratchLeaf);
		Fill(5, Directory(6, cache));
		Fill(4, Directory(5, cache));
		Fill(0, Directory(4, cache));
		Fill(1, Directory(5, cache));
		Fill(2, Directory(6, cache));
		Fill(3, scratchLeaf);
		uint16_t indices[4];
		intel_n100_ppgtt_indices(window, indices);
		fPages[0].cpu[indices[0]] = Directory(1, cache);
		fPages[1].cpu[indices[1]] = Directory(2, cache);
		fPages[2].cpu[indices[2]] = Directory(3, cache);
		for (unsigned i = 0; i < dataPages; i++)
			intel_n100_ppgtt_leaf(fPages[8 + i].dma, dataPat, true, &fPages[3].cpu[i]);
		fState = Ready;
		return true;
	}

	// Legal only because this object has no publication operation. Never use
	// this as post-submission cleanup without a separate retirement contract.
	bool Release()
	{
		if (fState == Ambiguous) return false;
		bool complete = true;
		for (unsigned i = 0; i < fCount; i++) {
			if (!fPages[i].token) continue;
			if (fBackend.free(fBackend.context, fPages[i].token))
				fPages[i] = {};
			else
				complete = false;
		}
		if (!complete) {
			fState = Retained;
			return false;
		}
		if (fHeld) fBackend.release(fBackend.context);
		fHeld = false;
		fCount = 0;
		fState = Empty;
		return true;
	}
private:
	IntelN100PpgttTree(const IntelN100PpgttTree&) = delete;
	IntelN100PpgttTree& operator=(const IntelN100PpgttTree&) = delete;
	bool ValidPage(unsigned index) const
	{
		const Page& page = fPages[index];
		uintptr_t cpu = reinterpret_cast<uintptr_t>(page.cpu);
		if (!page.token || !cpu || (cpu & 4095) != 0
			|| cpu > UINTPTR_MAX - 4095 || !intel_n100_ppgtt_page(page.dma))
			return false;
		// Exact4K aligned ranges overlap iff their starting addresses match.
		for (unsigned i = 0; i < index; i++) {
			if (page.token == fPages[i].token || page.cpu == fPages[i].cpu
				|| page.dma == fPages[i].dma)
				return false;
		}
		return true;
	}
	uint64_t Directory(unsigned page, IntelN100DirectoryCache cache) const
	{
		uint64_t entry = 0;
		intel_n100_ppgtt_directory(fPages[page].dma, cache, &entry);
		return entry;
	}
	void Fill(unsigned page, uint64_t entry)
	{
		for (unsigned i = 0; i < 512; i++) fPages[page].cpu[i] = entry;
	}
	Backend fBackend;
	Page fPages[24];
	unsigned fCount;
	bool fHeld;
	State fState;
};
#endif
