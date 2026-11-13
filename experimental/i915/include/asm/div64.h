/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_DIV64_H
#define RENKU_ASM_DIV64_H
#include <kpi_base.h>
#ifndef BITS_PER_LONG
#define BITS_PER_LONG 64
#endif
#if BITS_PER_LONG != 64
#error Native division adapter requires 64-bit unsigned long
#endif
#include "../../upstream/asm-generic/div64.h"
#endif
