/* SPDX-License-Identifier: MIT */
#ifndef RENKU_GFP_TYPES_H
#define RENKU_GFP_TYPES_H
#include <kpi_base.h>
typedef unsigned int gfp_t;
/* Original flag vocabulary does not supply Linux allocation policy. Native
 * allocators still reject unsupported flag combinations explicitly. */
#include "../../upstream/linux/gfp_types.h"
#endif
