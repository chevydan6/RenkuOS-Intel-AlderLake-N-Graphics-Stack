/* SPDX-License-Identifier: MIT */
#ifndef RENKU_UAPI_KDEV_H
#define RENKU_UAPI_KDEV_H
/* Select the kernel side of this include-only UAPI header. No surrounding
 * Linux kernel configuration is enabled by this narrowly scoped selection. */
#ifndef __KERNEL__
#define RENKU_KDEV_TEMP_KERNEL
#define __KERNEL__
#endif
#include "../../../upstream/uapi/linux/kdev_t.h"
#ifdef RENKU_KDEV_TEMP_KERNEL
#undef __KERNEL__
#undef RENKU_KDEV_TEMP_KERNEL
#endif
#endif
