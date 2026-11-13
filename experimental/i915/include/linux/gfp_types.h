/* SPDX-License-Identifier: MIT */
#ifndef RENKU_GFP_TYPES_H
#define RENKU_GFP_TYPES_H
/* Linux v6.6 gfp_types.h values; unsupported allocation modes fail. */
typedef unsigned int gfp_t;
#define __GFP_DIRECT_RECLAIM 0x400u
#define __GFP_ZERO 0x100u
#define GFP_KERNEL 0xcc0u
#define GFP_ATOMIC 0x820u
#endif
