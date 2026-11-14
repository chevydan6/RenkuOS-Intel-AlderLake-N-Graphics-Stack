// SPDX-License-Identifier: MIT
#include <IntelN100PpgttTree.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

struct FakePages {
 alignas(4096) uint64_t cpu[24][512];
 unsigned next=0;
 bool held=false;
 static bool Hold(void* context) {
  auto& self=*(FakePages*)context;
  if(self.held)return false;
  self.held=true;return true;
 }
 static void Drop(void* context) {((FakePages*)context)->held=false;}
 static bool Allocate(void* context,IntelN100PpgttTree::Page& page) {
  auto& self=*(FakePages*)context;
  if(self.next>=16)return false;
  unsigned index=self.next++;
  page={index+1,self.cpu[index],0x100000+uint64_t(index)*4096};
  return true;
 }
 static bool Free(void*,uint64_t) {return true;}
 IntelN100PpgttTree::Page External(unsigned index) {
  return {index+1,cpu[index],0x100000+uint64_t(index)*4096};
 }
};

int main()
{
 FakePages fake;
 IntelN100PpgttTree tree;
 IntelN100PpgttTree::Backend backend={&fake,FakePages::Hold,FakePages::Drop,
  FakePages::Allocate,FakePages::Free};
 assert(tree.Build(backend,0x400000,8,3,3,IntelN100DirectoryUncached));
 IntelN100PpgttTree::Page table={},scratch={};
 assert(tree.Describe(3,table) && tree.Describe(7,scratch));
 uint64_t scratchLeaf=0,leaf0=0,leaf1=0;
 assert(intel_n100_ppgtt_leaf(scratch.dma,3,true,&scratchLeaf));
 assert(table.cpu[8]==scratchLeaf && table.cpu[9]==scratchLeaf);
 auto first=fake.External(16),second=fake.External(17);
 IntelN100PpgttTree::Page pair[2]={first,second};
 assert(!tree.BindPages(0x407000,pair,2,3)); // Fixed workload is protected.
 assert(!tree.BindPages(0x408001,pair,2,3));
 assert(!tree.BindPages(0x408000,pair,2,8));
 assert(!tree.BindPages(0x408000,nullptr,2,3));
 assert(!tree.BindPages(0x408000,pair,0,3));
 assert(!tree.BindPages(0x600000,pair,2,3));
 assert(!tree.BindPages(0x5ff000,pair,2,3));
 assert(table.cpu[8]==scratchLeaf && table.cpu[9]==scratchLeaf);
 IntelN100PpgttTree::Page duplicate[2]={first,first};
 assert(!tree.BindPages(0x408000,duplicate,2,3));
 IntelN100PpgttTree::Page badDma[2]={first,second};badDma[1].dma=0x110001;
 assert(!tree.BindPages(0x408000,badDma,2,3));
 assert(table.cpu[8]==scratchLeaf && table.cpu[9]==scratchLeaf);
 assert(tree.BindPages(0x408000,pair,2,3));
 assert(intel_n100_ppgtt_leaf(first.dma,3,true,&leaf0));
 assert(intel_n100_ppgtt_leaf(second.dma,3,true,&leaf1));
 assert(table.cpu[8]==leaf0 && table.cpu[9]==leaf1);
 assert(!tree.BindPages(0x408000,pair,2,3));
 assert(!tree.BindPages(0x40a000,&first,1,3)); // Same DMA cannot alias.
 auto third=fake.External(18);
	assert(!tree.BindingsSealed());
	assert(tree.SealBindings());
	assert(tree.BindingsSealed());
 assert(!tree.BindPages(0x40a000,&third,1,3));
 assert(table.cpu[10]==scratchLeaf);
 puts("N100 PPGTT prepublication binding and seal: PASS (CPU only)");
}
