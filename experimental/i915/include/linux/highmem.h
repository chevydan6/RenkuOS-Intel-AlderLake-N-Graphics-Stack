/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_HIGHMEM_H
#define RENKU_LINUX_HIGHMEM_H
#include <linux/mm.h>
#include <linux/preempt.h>
#include <linux/uaccess.h>
#if defined(CONFIG_HIGHMEM) || defined(CONFIG_PREEMPT_RT) || defined(CONFIG_KMAP_LOCAL) || defined(ARCH_HAS_FLUSH_ON_KUNMAP)
#error This backend requires non-RT wired direct-map pages without highmem/cache aliases
#endif
/* Unused RT branches must parse but cannot bind an unrelated native symbol. */
void renku_linux_migrate_disable(void);
void renku_linux_migrate_enable(void);
#define migrate_disable renku_linux_migrate_disable
#define migrate_enable renku_linux_migrate_enable
#include "../../build/highmem_atomic.h"
#endif
