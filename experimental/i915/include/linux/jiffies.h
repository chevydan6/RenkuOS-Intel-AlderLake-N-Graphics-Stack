/* SPDX-License-Identifier: MIT */
#ifndef RENKU_JIFFIES_H
#define RENKU_JIFFIES_H
#include <kpi_base.h>
/* Explicit port tick unit. Native monotonic microseconds back all deadlines. */
#define HZ 1000
#define MAX_JIFFY_OFFSET ((LONG_MAX >> 1) - 1)
static inline unsigned long msecs_to_jiffies(unsigned int ms) { return ms; }
#endif
