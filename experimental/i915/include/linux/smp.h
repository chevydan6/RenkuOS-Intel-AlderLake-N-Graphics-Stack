/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SMP_H
#define RENKU_LINUX_SMP_H
#include <kpi_base.h>
#ifdef RENKU_NATIVE
/* Exported native prototypes from private/kernel/smp.h. */
extern int32 smp_get_current_cpu(void);
extern int32 smp_get_num_cpus(void);
#define raw_smp_processor_id smp_get_current_cpu
#define smp_processor_id smp_get_current_cpu
#endif
#endif
