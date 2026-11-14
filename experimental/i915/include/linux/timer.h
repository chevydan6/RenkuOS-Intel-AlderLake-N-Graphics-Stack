/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_TIMER_H
#define RENKU_LINUX_TIMER_H
#include <kpi_base.h>
/* Native add_timer and POSIX timer_delete are different APIs. Keep Linux
 * callers in their own namespace; actual Linux timer delivery/cancellation
 * still requires a native backend and remains unresolved at link time. */
#define add_timer renku_linux_add_timer
#define timer_delete renku_linux_timer_delete
#include "../../upstream/linux/timer.h"
#endif
