/* SPDX-License-Identifier: MIT */
#ifndef RENKU_PORT_CONFIG_H
#define RENKU_PORT_CONFIG_H
/* Configuration of this x86-64 native compatibility build, not a Linux kernel. */
#if !defined(__x86_64__)
#error Only x86-64 is currently supported
#endif
#define CONFIG_64BIT 1
#define CONFIG_X86_64 1
#define CONFIG_X86 1
#define CONFIG_SMP 1
/* Supported native EFI/x86-64 baseline has SMP_MAX_CPUS=64. Reject larger
 * native CPU counts at backend startup instead of truncating CPU identities. */
#define CONFIG_NR_CPUS 64
#define CONFIG_X86_L1_CACHE_SHIFT 6
#define CONFIG_X86_INTERNODE_CACHE_SHIFT 6
#define CONFIG_HZ 1000
#define __EXPORTED_HEADERS__ 1
#endif
