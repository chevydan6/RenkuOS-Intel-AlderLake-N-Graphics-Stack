/* SPDX-License-Identifier: MIT */
#ifndef RENKU_NATIVE_PAGES_H
#define RENKU_NATIVE_PAGES_H
#include <linux/mm.h>
/* Serialized module lifecycle; owner excludes new/running users at shutdown.
 * Internal status is Linux errno, never a native status_t result. */
int renku_linux_pages_init(void);
int renku_linux_pages_drain(unsigned int timeout_ms);
int renku_linux_pages_uninit(void);
void renku_linux_pagefault_disable(void);
void renku_linux_pagefault_enable(void);
bool renku_linux_pagefault_disabled(void);
struct renku_page_store;
/* Sparse native anonymous backing, not a Linux file/address_space layout.
 * Size is page aligned. The owner excludes new callers before destroy; destroy
 * refuses live folios, in-flight pins and queued unpins. Returned folios own a
 * writable native pin. Their final put queues unpin, retaining store contents. */
struct renku_page_store *renku_linux_page_store_create(size_t size);
struct folio *renku_linux_page_store_read(struct renku_page_store *, unsigned long index);
int renku_linux_page_store_destroy(struct renku_page_store *);
#endif
