/* SPDX-License-Identifier: MIT */
#ifndef RENKU_BITSPERLONG_H
#define RENKU_BITSPERLONG_H
#if !defined(__x86_64__)
#error Only native x86-64 bit widths are supported
#endif
#include <linux/kconfig.h>
#include "../../upstream/asm-generic/bitsperlong.h"
#endif
