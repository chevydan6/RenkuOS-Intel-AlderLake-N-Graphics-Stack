/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SWAP_H
#define RENKU_LINUX_SWAP_H
#include <linux/mm.h>
#include <linux/pagevec.h>
/* Driver-facing interfaces only. RenkuOS owns paging; Linux's global zone,
 * swap and VM flag layouts are not part of this adapter's public ABI.
 * These services stay unresolved until their native implementations exist. */
void mark_page_accessed(struct page *);
void folio_mark_accessed(struct folio *);
void lru_add_drain(void);
void lru_add_drain_all(void);
void check_move_unevictable_folios(struct folio_batch *);
#endif
