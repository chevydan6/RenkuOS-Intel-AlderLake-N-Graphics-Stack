/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_MM_H
#define RENKU_LINUX_MM_H
#include <kpi_base.h>
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
struct address_space;
struct vm_area_struct;
/* Unresolved VM operation, not an implementation or successful no-op. */
void unmap_mapping_range(struct address_space *, s64, s64, int);
#endif
