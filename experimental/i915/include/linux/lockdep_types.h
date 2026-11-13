/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LOCKDEP_TYPES_WRAPPER_H
#define RENKU_LOCKDEP_TYPES_WRAPPER_H
#if defined(CONFIG_LOCKDEP) || defined(CONFIG_DEBUG_LOCK_ALLOC)
#error Linux lock dependency instrumentation is not ported
#endif
#include <kpi_base.h>
#include "../../upstream/linux/lockdep_types.h"
#endif
