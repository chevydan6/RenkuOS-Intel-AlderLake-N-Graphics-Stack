/* SPDX-License-Identifier: MIT */
#include <kpi_base.h>
#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
