/* SPDX-License-Identifier: MIT */
#ifndef RENKU_KDEV_H
#define RENKU_KDEV_H
#include <kpi_base.h>
/* Native dev_t is signed and belongs to another ID domain. Original Linux
 * helpers use an unsigned 32-bit type; callers must use unsigned Linux IDs.
 * No native filesystem/device-ID conversion is implemented here. */
typedef u32 renku_linux_dev_t;
#define dev_t renku_linux_dev_t
#include "../../upstream/linux/kdev_t.h"
#undef dev_t
#endif
