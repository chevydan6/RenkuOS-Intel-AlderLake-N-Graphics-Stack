/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_MM_H
#define RENKU_LINUX_MM_H
#include <kpi_base.h>
#include <linux/errno.h>
#define PAGE_SHIFT 12
#ifdef RENKU_NATIVE
#define PAGE_SIZE B_PAGE_SIZE
_Static_assert(B_PAGE_SIZE == (1UL << PAGE_SHIFT), "Expected native x86-64 page size");
#else
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#ifndef BITS_PER_LONG
#define BITS_PER_LONG 64
#endif
/* Native page metadata view for Linux interfaces. The owner must keep backing
 * wired through DMA unmap. General Linux folio/page allocation remains separate. */
struct page { void *virtual; phys_addr_t physical; };
struct folio { struct page page; };
struct page *virt_to_page(const void *);
struct page *nth_page(struct page *, unsigned long);
static inline void *page_address(const struct page *page) { return page->virtual; }
static inline phys_addr_t page_to_phys(const struct page *page) { return page->physical; }
#define offset_in_page(p) ((unsigned long)(p) & (PAGE_SIZE - 1))
struct address_space;
struct vm_area_struct;
/* Unresolved VM operation, not an implementation or successful no-op. */
void unmap_mapping_range(struct address_space *, s64, s64, int);
#endif
