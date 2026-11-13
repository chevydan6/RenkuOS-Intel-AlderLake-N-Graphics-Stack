/* SPDX-License-Identifier: MIT */
/* GCC -Wextra diagnoses the original GENMASK(size - 1, 0) constant input
 * check as an unsigned comparison against zero. Keep the check unchanged
 * (including invalid-mask build errors), and scope this warning to this header. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#include "../../upstream/linux/find.h"
#pragma GCC diagnostic pop
