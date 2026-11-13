/* SPDX-License-Identifier: MIT */
#ifndef RENKU_GFP_H
#define RENKU_GFP_H
#include <linux/gfp_types.h>
/* Original blocking predicate plus native owned-page allocation. Zone
 * selection/reclaim and Linux VM layouts remain separate services. */
#include "../../build/gfp_helpers.h"
struct page;
struct page *alloc_pages(gfp_t, unsigned int);
struct folio;
struct folio *folio_alloc(gfp_t, unsigned int);
#define alloc_page(flags) alloc_pages((flags), 0)
#endif
