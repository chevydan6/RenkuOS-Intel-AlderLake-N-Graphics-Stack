/* SPDX-License-Identifier: MIT */
#ifndef RENKU_COMPILER_ATTRIBUTES_WRAPPER_H
#define RENKU_COMPILER_ATTRIBUTES_WRAPPER_H
/* Native libc/kernel declarations are parsed before the Linux attribute
 * namespace is selected. Both forms express compiler inlining. */
#undef __always_inline
#undef __attribute_const__
#undef __packed
#include "../../upstream/linux/compiler_attributes.h"
#endif
