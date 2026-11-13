/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_PROCESSOR_H
#define RENKU_ASM_PROCESSOR_H
#include <kpi_base.h>
/* Existing x86-64 CPU relaxation instruction, shared by scheduler/seq readers.
 * No Linux task, TSS, processor-info or address-space layout is supplied. */
static inline void cpu_relax(void)
{
	__asm__ __volatile__("pause" ::: "memory");
}
#endif
