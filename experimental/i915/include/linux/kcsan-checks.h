/* SPDX-License-Identifier: MIT */
#ifndef RENKU_KCSAN_CHECKS_H
#define RENKU_KCSAN_CHECKS_H
#include <kpi_base.h>
#if defined(CONFIG_KCSAN) || defined(CONFIG_KCSAN_WEAK_MEMORY)
#error Linux KCSAN runtime instrumentation is not ported
#endif
/* Original configuration-disabled instrumentation only; not race detection. */
#include "../../upstream/linux/kcsan-checks.h"
#endif
