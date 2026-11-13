/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_RTMUTEX_H
#define RENKU_LINUX_RTMUTEX_H
/* ww_mutex.h includes rtmutex.h unconditionally, but its non-RT branch uses
 * struct mutex. Provide no RT API, rather than fake priority inheritance. */
#ifdef CONFIG_PREEMPT_RT
#error PREEMPT_RT mutex support is excluded from this experiment
#endif
#endif
