/* SPDX-License-Identifier: MIT */
#ifndef RENKU_GFP_H
#define RENKU_GFP_H
#include <linux/gfp_types.h>
/* Complete original blocking predicate only. Zone selection and page
 * allocation stay unresolved; no Linux VM layout is synthesized. */
#include "../../build/gfp_helpers.h"
#endif
