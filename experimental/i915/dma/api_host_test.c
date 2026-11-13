/* SPDX-License-Identifier: MIT */
#include <linux/dma-mapping.h>
#include <assert.h>
#include <stdlib.h>
#include "bus_dma_bridge.h"
struct device { int test_only; };
static int calls, fail_at, live, warnings, allocs;
static const void *keys[8];
static unsigned char memory[4][4096];
bool renku_kpi_warn(const char *format, ...) { (void)format; warnings++; return true; }
void panic(const char *format, ...) { (void)format; abort(); }
int renku_bus_dma_set_mask(const void *device, uint64_t mask)
{ return device && mask ? 0 : -1; }
void *renku_bus_dma_alloc(const void *device, size_t size, uint64_t *dma)
{ if (!device || !size) return NULL; allocs++; *dma=0x100000; return memory; }
int renku_bus_dma_free(const void *device, size_t size, void *cpu, uint64_t dma)
{ assert(device && size && cpu==memory && dma==0x100000); allocs--; return 0; }
int renku_bus_dma_map(const void *device, const void *key, void *cpu, size_t size,
 int direction, int skip, uint64_t *dma)
{
 assert(device && cpu && size && direction==DMA_BIDIRECTIONAL && skip==1);
 calls++;
 if (calls==fail_at) return -1;
 for (unsigned i=0;i<8;i++) if (keys[i]==key) return -1;
 for (unsigned i=0;i<8;i++) if (!keys[i]) { keys[i]=key; break; }
 live++; *dma=0x200000+(calls*4096); return 0;
}
int renku_bus_dma_unmap(const void *device, const void *key, size_t size,
 int direction, int skip)
{
 assert(device && size && skip==1);
 if (direction!=DMA_BIDIRECTIONAL) return -1;
 for (unsigned i=0;i<8;i++) if (keys[i]==key) { keys[i]=NULL; live--; return 0; }
 return -1;
}
static void Build(struct scatterlist *first, struct scatterlist *last, struct page *pages)
{
 sg_init_table(first,3);sg_init_table(last,2);
 sg_set_page(&first[0],&pages[0],4096,0);
 sg_set_page(&first[1],&pages[1],1024,64);
 sg_chain(first,3,last);
 sg_set_page(&last[0],&pages[2],2048,0);
 sg_set_page(&last[1],&pages[3],4096,0);
}
int main(void)
{
 struct device device={}; struct page pages[4];
 for(unsigned i=0;i<4;i++) pages[i]=(struct page){.virtual=memory[i],.physical=0x400000+i*4096};
 const unsigned long attrs=DMA_ATTR_SKIP_CPU_SYNC|DMA_ATTR_NO_KERNEL_MAPPING|DMA_ATTR_NO_WARN;
 for(int fail=0;fail<=4;fail++) {
  struct scatterlist first[3],last[2];Build(first,last,pages);
  calls=0;fail_at=fail;
  unsigned mapped=dma_map_sg_attrs(&device,first,4,DMA_BIDIRECTIONAL,attrs);
  if(!fail) {
   assert(mapped==4 && live==4);
   assert(dma_map_sg_attrs(&device,first,4,DMA_BIDIRECTIONAL,attrs)==0 && live==4);
   dma_unmap_sg_attrs(&device,first,4,DMA_BIDIRECTIONAL,attrs);
  } else assert(mapped==0 && live==0 && calls==fail);
  assert(live==0);
 }
 struct scatterlist first[3],last[2];Build(first,last,pages);fail_at=0;calls=0;
 first[1].length=0;
 assert(dma_map_sg_attrs(&device,first,4,DMA_BIDIRECTIONAL,attrs)==0 && live==0);
 assert(sg_dma_len(first)==0);
 Build(first,last,pages);
 assert(dma_map_sg_attrs(&device,first,4,DMA_NONE,attrs)==0);
 assert(dma_map_sg_attrs(&device,first,4,DMA_BIDIRECTIONAL,1UL<<30)==0);
 dma_addr_t address=0xdeadbeef;
 assert(!dma_alloc_coherent(&device,4096,&address,GFP_ATOMIC) && address==0xdeadbeef);
 assert(!dma_alloc_attrs(&device,4096,&address,GFP_KERNEL,DMA_ATTR_NO_KERNEL_MAPPING));
 assert(!dma_alloc_coherent(NULL,4096,&address,GFP_KERNEL) && address==0xdeadbeef);
 void *cpu=dma_alloc_coherent(&device,4096,&address,GFP_KERNEL);
 assert(cpu && address==0x100000 && allocs==1);
 dma_free_coherent(&device,4096,cpu,address);assert(allocs==0 && warnings==0);
 return 0;
}
