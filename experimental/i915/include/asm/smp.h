/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_SMP_H
#define RENKU_ASM_SMP_H
#include <linux/smp.h>
/* Requires actual cache invalidation on every online CPU, not a local fence. */
void wbinvd_on_all_cpus(void);
#endif
