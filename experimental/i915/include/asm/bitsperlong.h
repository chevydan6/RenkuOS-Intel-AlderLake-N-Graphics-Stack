/* SPDX-License-Identifier: MIT */
#ifndef RENKU_BITSPERLONG_H
#define RENKU_BITSPERLONG_H
#if !defined(__x86_64__)
#error Only native x86-64 bit widths are supported
#endif
#define __BITS_PER_LONG 64
#ifndef BITS_PER_LONG
#define BITS_PER_LONG 64
#endif
#endif
