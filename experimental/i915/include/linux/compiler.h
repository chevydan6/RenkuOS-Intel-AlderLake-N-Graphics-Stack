/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_COMPILER_H
#define RENKU_LINUX_COMPILER_H
#include <kpi_base.h>
/* Compiler-only barrier, matching Linux's GCC compiler barrier contract. */
#define barrier() __asm__ __volatile__("" ::: "memory")
#endif
