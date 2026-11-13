/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_KTIME_H
#define RENKU_LINUX_KTIME_H
#include <kpi_base.h>
typedef s64 ktime_t;
ktime_t ktime_get(void);
#endif
