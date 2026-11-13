/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_LINKAGE_H
#define RENKU_ASM_LINKAGE_H
#if !defined(__x86_64__) || defined(__ASSEMBLY__)
#error This port currently supports x86-64 C calling conventions only
#endif
/* Native and Linux x86-64 C functions both use the compiler's default ABI.
 * linux/linkage.h supplies CPP_ASMLINKAGE; Linux assembler/CFI entry macros
 * are deliberately not exposed to native kernel code. */
#ifdef __cplusplus
#define asmlinkage extern "C"
#else
#define asmlinkage
#endif
#endif
