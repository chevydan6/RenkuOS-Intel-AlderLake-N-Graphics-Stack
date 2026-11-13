/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LOCKREF_H
#define RENKU_LOCKREF_H
#include <linux/spinlock.h>
/* Only the original lock-based algorithm is supported. Its disabled cmpxchg
 * branch does not need SPINLOCK_SIZE or Linux VM bounds. */
#define RENKU_IMPORT_LOCKREF
#include "../../upstream/linux/lockref.h"
#undef RENKU_IMPORT_LOCKREF
#if USE_CMPXCHG_LOCKREF
#error Native lock scopes require the original spinlock fallback
#endif
#endif
