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
#endif
