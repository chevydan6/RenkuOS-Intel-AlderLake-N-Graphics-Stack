/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_MM_H
#define RENKU_LINUX_MM_H
#include <kpi_base.h>
#include <linux/errno.h>
#define PAGE_SHIFT 12
#ifdef RENKU_NATIVE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
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
/* Canonical head view only, not the full Linux folio layout. Its actual
 * size/reference state is owned by the native allocation registry. */
struct folio { struct page page; };
_Static_assert(sizeof(struct folio) == sizeof(struct page) && offsetof(struct folio, page) == 0,
	"folio head view must alias exactly one page descriptor");
/* Managed pages have native wired backing and explicit reference lifetime.
 * Borrowed legacy DMA views remain caller-owned and cannot acquire references. */
struct folio *renku_linux_page_folio(const struct page *);
unsigned int folio_order(struct folio *);
long folio_nr_pages(struct folio *);
bool folio_test_large(struct folio *);
unsigned int compound_order(struct page *);
int PageHead(const struct page *);
int PageTail(const struct page *);
int PageCompound(const struct page *);
bool folio_try_get(struct folio *);
void folio_get(struct folio *);
void folio_put(struct folio *);
int folio_ref_count(const struct folio *);
struct page *compound_head(struct page *);
bool get_page_unless_zero(struct page *);
void get_page(struct page *);
void put_page(struct page *);
unsigned int page_count(const struct page *);
void __free_pages(struct page *, unsigned int);
#define __free_page(p) __free_pages((p), 0)
struct page *virt_to_page(const void *);
struct page *nth_page(struct page *, unsigned long);
static inline void *page_address(const struct page *page) { return page->virtual; }
static inline phys_addr_t page_to_phys(const struct page *page) { return page->physical; }
static inline unsigned long page_to_pfn(const struct page *page) { return page_to_phys(page) >> PAGE_SHIFT; }
#define offset_in_page(p) ((unsigned long)(p) & (PAGE_SIZE - 1))
#include "../../build/folio_helpers.h"
struct address_space;
struct vm_area_struct;
/* Unresolved VM operation, not an implementation or successful no-op. */
void unmap_mapping_range(struct address_space *, s64, s64, int);
#endif
