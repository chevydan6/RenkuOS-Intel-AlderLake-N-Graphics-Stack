/* SPDX-License-Identifier: MIT */
#include <kpi_base.h>
/* glibc itself requests linux/limits.h while its native limits are loading.
 * Do not expose kernel typedefs or cast-valued limits during that bootstrap. */
#ifndef RENKU_KPI_BASE_READY
#include <uapi/linux/limits.h>
#else
#ifndef RENKU_LIMITS_WRAPPER_H
#define RENKU_LIMITS_WRAPPER_H
#include <linux/types.h>
#undef U64_MAX
#undef SIZE_MAX
#undef SSIZE_MAX
#include "../../upstream/linux/limits.h"
#endif
#endif
