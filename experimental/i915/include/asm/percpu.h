/* SPDX-License-Identifier: MIT */
#ifndef RENKU_PERCPU_ANNOTATIONS_H
#define RENKU_PERCPU_ANNOTATIONS_H
/* lockdep-off headers include this unconditionally. __percpu is the original
 * compiler annotation. No Linux per-CPU allocation/access operations supplied. */
#include <linux/compiler_types.h>
#endif
