/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SHMEM_FS_H
#define RENKU_LINUX_SHMEM_FS_H
#include <linux/file.h>
#include <linux/err.h>
/* Linux v6.6 include/linux/mm.h: suppress up-front backing commitment. */
#ifndef VM_NORESERVE
#define VM_NORESERVE 0x00200000
#endif
struct file *shmem_file_setup(const char *, loff_t, unsigned long);
struct folio *shmem_read_folio_gfp(struct address_space *, pgoff_t, gfp_t);
struct page *shmem_read_mapping_page_gfp(struct address_space *, pgoff_t, gfp_t);
static inline struct page *shmem_read_mapping_page(struct address_space *mapping, pgoff_t index)
{ return shmem_read_mapping_page_gfp(mapping, index, mapping_gfp_mask(mapping)); }
/* Reclaim/writeback, truncate, dirty/accessed, unevictability and user mappings
 * require real native contracts. They deliberately remain unimplemented. */
#endif
