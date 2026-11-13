/* SPDX-License-Identifier: MIT */
#include <kpi_base.h>
#ifndef RENKU_VDSO_LIMITS_H
#define RENKU_VDSO_LIMITS_H
#undef USHRT_MAX
#undef SHRT_MAX
#undef SHRT_MIN
#undef INT_MAX
#undef INT_MIN
#undef UINT_MAX
#undef LONG_MAX
#undef LONG_MIN
#undef ULONG_MAX
#undef LLONG_MAX
#undef LLONG_MIN
#undef ULLONG_MAX
#undef UINTPTR_MAX
#include "../../upstream/vdso/limits.h"
#endif
