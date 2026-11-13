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
#define __EXPORTED_HEADERS__ 1
#endif
