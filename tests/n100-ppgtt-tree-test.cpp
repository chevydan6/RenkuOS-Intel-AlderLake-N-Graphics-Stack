// SPDX-License-Identifier: MIT
#include "IntelN100PpgttTree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

using Tree = IntelN100PpgttTree;
struct Model {
	void* memory[24] = {};
	bool live[24] = {};
	unsigned calls = 0, frees = 0, holds = 0, drops = 0;
	int failAllocation = -1, failFree = -1, corrupt = -1;
	bool failRetain = false;
	static bool Retain(void* c) { Model& m = *(Model*)c; if (m.failRetain) return false; m.holds++; return true; }
	static void Drop(void* c) {
		Model& m = *(Model*)c;
		for (bool live : m.live) assert(!live);
		m.drops++;
	}
	static bool Allocate(void* c, Tree::Page& out) {
		Model& m = *(Model*)c;
		unsigned i = m.calls++;
		assert(i < 24 && m.holds == 1 && m.drops == 0);
		if (int(i) == m.failAllocation) return false;
		assert(posix_memalign(&m.memory[i], 4096, 4096) == 0);
		memset(m.memory[i], 0xa5, 4096);
		m.live[i] = true;
		out = {i + 1, (uint64_t*)m.memory[i], 0x100000 + i * 4096ULL};
		if (i == 1) {
			switch (m.corrupt) {
				case 0: out.token = 1; break;
				case 1: out.cpu = (uint64_t*)m.memory[0]; break;
				case 2: out.dma = 0x100000; break;
				case 3: out.dma = 1ULL << 39; break;
				case 4: out.cpu = (uint64_t*)((char*)out.cpu + 1); break;
				case 5: out.token = 0; break;
				case 6: out.cpu = nullptr; break;
				case 7: return false; // Deliberate allocator-contract violation.
			}
		}
		return true;
	}
	static bool Free(void* c, uint64_t token) {
		Model& m = *(Model*)c;
		assert(token >= 1 && token <= 24 && m.drops == 0);
		unsigned i = token - 1;
		assert(m.live[i]);
		m.frees++;
		if (int(i) == m.failFree) return false;
		m.live[i] = false;
		free(m.memory[i]); m.memory[i] = nullptr;
		return true;
	}
	Tree::Backend Backend() { return {this, Retain, Drop, Allocate, Free}; }
};

static uint64_t Dma(unsigned i) { return 0x100000 + i * 4096ULL; }
static void CheckTree(Tree& tree, Model& model, uint64_t window, unsigned count,
	uint64_t directoryFlags = 3, uint64_t dataFlags = 3, uint64_t scratchFlags = 0x1b)
{
	assert(tree.CurrentState() == Tree::Ready);
	uint64_t root = 0;
	assert(tree.Root(root) && root == Dma(0));
	unsigned slots[] = {unsigned((window >> 39) & 511),
		unsigned((window >> 30) & 511), unsigned((window >> 21) & 511)};
	for (unsigned page = 0; page < 7; page++) {
		for (unsigned slot = 0; slot < 512; slot++) {
			uint64_t expected;
			if (page < 3) expected = Dma(slot == slots[page] ? page + 1 : page + 4) | directoryFlags;
			else if (page == 3) expected = Dma(slot < count ? 8 + slot : 7)
				| (slot < count ? dataFlags : scratchFlags);
			else if (page < 6) expected = Dma(page + 1) | directoryFlags;
			else expected = Dma(7) | scratchFlags;
			assert(((uint64_t*)model.memory[page])[slot] == expected);
		}
	}
	for (unsigned page = 7; page < 8 + count; page++)
		for (unsigned byte = 0; byte < 4096; byte++)
			assert(((unsigned char*)model.memory[page])[byte] == 0);
	// Walk every page in the active window and one outside it independently.
	for (unsigned n = 0; n <= 512; n++) {
		uint64_t va = n < 512 ? window + n * 4096 : window ^ (1ULL << 39);
		uint64_t dma = root;
		for (unsigned shift = 39;; shift -= 9) {
			unsigned page = (dma - 0x100000) / 4096;
			assert(page < 7);
			uint64_t entry = ((uint64_t*)model.memory[page])[(va >> shift) & 511];
			dma = entry & ~4095ULL;
			if (shift == 12) break;
		}
		assert(dma == Dma(n < count ? 8 + n : 7));
	}
}

int main()
{
	unsigned cases = 0;
	const uint64_t windows[] = {0, (1ULL << 39) | (2ULL << 30) | (3ULL << 21), (1ULL << 48) - (1ULL << 21)};
	for (uint64_t window : windows) {
		Model model; Tree tree;
		assert(tree.Build(model.Backend(), window, 16, 0, 3, IntelN100DirectoryCached));
		CheckTree(tree, model, window, 16);
		assert(!tree.Build(model.Backend(), 0, 1, 0, 3, IntelN100DirectoryCached));
		assert(tree.Release() && tree.Release() && model.drops == 1 && model.frees == 24);
		uint64_t root = 91; assert(!tree.Root(root) && root == 91);
		cases++;
	}
	for (int fail = 0; fail < 24; fail++) {
		Model model; Tree tree; model.failAllocation = fail;
		assert(!tree.Build(model.Backend(), 0, 16, 0, 3, IntelN100DirectoryCached));
		assert(tree.CurrentState() == Tree::Empty && model.frees == unsigned(fail) && model.drops == 1);
		cases++;
	}
	{
		Model model; Tree tree;
		assert(tree.Build(model.Backend(), 0, 1, 1, 2, IntelN100DirectoryUncached));
		CheckTree(tree, model, 0, 1, 0x1b, 0x0b, 0x13);
		assert(tree.Release() && model.frees == 9 && model.drops == 1);
		cases++;
	}
	for (int fail = 0; fail < 24; fail++) {
		Model model; Tree tree; model.failFree = fail;
		assert(tree.Build(model.Backend(), 0, 16, 0, 3, IntelN100DirectoryCached));
		assert(!tree.Release() && tree.CurrentState() == Tree::Retained && model.drops == 0);
		assert(!tree.Build(model.Backend(), 0, 1, 0, 3, IntelN100DirectoryCached));
		for (unsigned i = 0; i < 24; i++) assert(model.live[i] == (int(i) == fail));
		model.failFree = -1;
		assert(tree.Release() && model.frees == 25 && model.drops == 1);
		cases++;
	}
	{
		Model model; Tree tree; model.failAllocation = 5; model.failFree = 2;
		assert(!tree.Build(model.Backend(), 0, 16, 0, 3, IntelN100DirectoryCached));
		assert(tree.CurrentState() == Tree::Retained && model.drops == 0);
		model.failFree = -1; assert(tree.Release() && model.drops == 1); cases++;
	}
	// Retain ambiguous records and callback contexts until simulated process end.
	static Model ambiguousModels[8]; static Tree ambiguousTrees[8];
	for (int corruption = 0; corruption < 8; corruption++) {
		Model& model = ambiguousModels[corruption]; Tree& tree = ambiguousTrees[corruption];
		model.corrupt = corruption;
		assert(!tree.Build(model.Backend(), 0, 16, 0, 3, IntelN100DirectoryCached));
		assert(tree.CurrentState() == Tree::Ambiguous && tree.RecordedPages() == 2);
		assert(!tree.Release() && model.frees == 0 && model.drops == 0);
		assert(!tree.Build(model.Backend(), 0, 1, 0, 3, IntelN100DirectoryCached));
		for (unsigned page = 0; page < 2; page++)
			for (unsigned byte = 0; byte < 4096; byte++) assert(((unsigned char*)model.memory[page])[byte] == 0xa5);
		uint64_t root = 91; assert(!tree.Root(root) && root == 91); cases++;
	}
	{
		Model model; Tree tree; model.failRetain = true;
		assert(!tree.Build(model.Backend(), 0, 1, 0, 3, IntelN100DirectoryCached));
		assert(model.calls == 0 && model.drops == 0 && tree.CurrentState() == Tree::Empty); cases++;
		model.failRetain = false;
		assert(!tree.Build(model.Backend(), 4096, 1, 0, 3, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 0, 0, 0, 3, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 0, 17, 0, 3, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 0, 1, 4, 3, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 0, 1, 0, 4, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 1ULL << 48, 1, 0, 3, IntelN100DirectoryCached));
		assert(!tree.Build(model.Backend(), 0, 1, 0, 3,
			static_cast<IntelN100DirectoryCache>(255)));
		assert(model.holds == 0); cases += 7;
	}
	printf("%u owned offline PPGTT construction cases passed; no GPU access.\n", cases);
}
