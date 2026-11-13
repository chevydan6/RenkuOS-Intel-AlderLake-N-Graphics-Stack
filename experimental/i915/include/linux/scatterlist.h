/* SPDX-License-Identifier: MIT */
#ifndef RENKU_SCATTERLIST_WRAPPER_H
#define RENKU_SCATTERLIST_WRAPPER_H
#ifndef RENKU_NATIVE
#include <sys/types.h>
#endif
#define CONFIG_NEED_SG_DMA_LENGTH 1
#include <linux/gfp_types.h>
#include "../../upstream/linux/scatterlist.h"
#endif
