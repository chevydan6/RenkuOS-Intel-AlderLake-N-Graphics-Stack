/*
 * xxHash - Extremely Fast Hash algorithm
 * Copyright (C) 2012-2023 Yann Collet
 *
 * BSD 2-Clause License (https://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RENKU_SELECTED_XXHASH_H
#define RENKU_SELECTED_XXHASH_H

/* Selected XXH32/XXH64 one-shot algorithms from Mesa 26.3.0's xxhash.h.
 * All current compiler callers define XXH_INLINE_ALL; no streaming or XXH3
 * symbols are in the linked N100 compiler closure. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t renku_xxh_rot32(uint32_t x, unsigned shift)
{ return (x << shift) | (x >> (32 - shift)); }

static inline uint64_t renku_xxh_rot64(uint64_t x, unsigned shift)
{ return (x << shift) | (x >> (64 - shift)); }

static inline uint32_t renku_xxh_read32(const uint8_t *p)
{
   uint32_t value;
   memcpy(&value, p, sizeof(value));
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
   value = __builtin_bswap32(value);
#endif
   return value;
}

static inline uint64_t renku_xxh_read64(const uint8_t *p)
{
   uint64_t value;
   memcpy(&value, p, sizeof(value));
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
   value = __builtin_bswap64(value);
#endif
   return value;
}

static inline uint32_t renku_xxh32_round(uint32_t value, uint32_t word)
{ return renku_xxh_rot32(value + word * UINT32_C(0x85ebca77), 13) * UINT32_C(0x9e3779b1); }

static inline uint32_t XXH32(const void *input, size_t length, uint32_t seed)
{
   const uint8_t *p = (const uint8_t *)input;
   size_t remaining = length;
   uint32_t hash;
   if (remaining >= 16) {
      uint32_t v1 = seed + UINT32_C(0x9e3779b1) + UINT32_C(0x85ebca77);
      uint32_t v2 = seed + UINT32_C(0x85ebca77);
      uint32_t v3 = seed;
      uint32_t v4 = seed - UINT32_C(0x9e3779b1);
      do {
         v1 = renku_xxh32_round(v1, renku_xxh_read32(p)); p += 4;
         v2 = renku_xxh32_round(v2, renku_xxh_read32(p)); p += 4;
         v3 = renku_xxh32_round(v3, renku_xxh_read32(p)); p += 4;
         v4 = renku_xxh32_round(v4, renku_xxh_read32(p)); p += 4;
         remaining -= 16;
      } while (remaining >= 16);
      hash = renku_xxh_rot32(v1, 1) + renku_xxh_rot32(v2, 7)
           + renku_xxh_rot32(v3, 12) + renku_xxh_rot32(v4, 18);
   } else {
      hash = seed + UINT32_C(0x165667b1);
   }
   hash += (uint32_t)length;
   while (remaining >= 4) {
      hash = renku_xxh_rot32(hash + renku_xxh_read32(p) * UINT32_C(0xc2b2ae3d), 17)
           * UINT32_C(0x27d4eb2f);
      p += 4;
      remaining -= 4;
   }
   while (remaining) {
      hash = renku_xxh_rot32(hash + *p++ * UINT32_C(0x165667b1), 11)
           * UINT32_C(0x9e3779b1);
      --remaining;
   }
   hash ^= hash >> 15; hash *= UINT32_C(0x85ebca77);
   hash ^= hash >> 13; hash *= UINT32_C(0xc2b2ae3d);
   return hash ^ (hash >> 16);
}

static inline uint64_t renku_xxh64_round(uint64_t value, uint64_t word)
{ return renku_xxh_rot64(value + word * UINT64_C(0xc2b2ae3d27d4eb4f), 31)
       * UINT64_C(0x9e3779b185ebca87); }

static inline uint64_t renku_xxh64_merge(uint64_t hash, uint64_t value)
{ return (hash ^ renku_xxh64_round(0, value)) * UINT64_C(0x9e3779b185ebca87)
       + UINT64_C(0x85ebca77c2b2ae63); }

static inline uint64_t XXH64(const void *input, size_t length, uint64_t seed)
{
   const uint8_t *p = (const uint8_t *)input;
   size_t remaining = length;
   uint64_t hash;
   if (remaining >= 32) {
      uint64_t v1 = seed + UINT64_C(0x9e3779b185ebca87)
                        + UINT64_C(0xc2b2ae3d27d4eb4f);
      uint64_t v2 = seed + UINT64_C(0xc2b2ae3d27d4eb4f);
      uint64_t v3 = seed;
      uint64_t v4 = seed - UINT64_C(0x9e3779b185ebca87);
      do {
         v1 = renku_xxh64_round(v1, renku_xxh_read64(p)); p += 8;
         v2 = renku_xxh64_round(v2, renku_xxh_read64(p)); p += 8;
         v3 = renku_xxh64_round(v3, renku_xxh_read64(p)); p += 8;
         v4 = renku_xxh64_round(v4, renku_xxh_read64(p)); p += 8;
         remaining -= 32;
      } while (remaining >= 32);
      hash = renku_xxh_rot64(v1, 1) + renku_xxh_rot64(v2, 7)
           + renku_xxh_rot64(v3, 12) + renku_xxh_rot64(v4, 18);
      hash = renku_xxh64_merge(hash, v1);
      hash = renku_xxh64_merge(hash, v2);
      hash = renku_xxh64_merge(hash, v3);
      hash = renku_xxh64_merge(hash, v4);
   } else {
      hash = seed + UINT64_C(0x27d4eb2f165667c5);
   }
   hash += length;
   while (remaining >= 8) {
      hash ^= renku_xxh64_round(0, renku_xxh_read64(p));
      hash = renku_xxh_rot64(hash, 27) * UINT64_C(0x9e3779b185ebca87)
           + UINT64_C(0x85ebca77c2b2ae63);
      p += 8;
      remaining -= 8;
   }
   if (remaining >= 4) {
      hash ^= (uint64_t)renku_xxh_read32(p) * UINT64_C(0x9e3779b185ebca87);
      hash = renku_xxh_rot64(hash, 23) * UINT64_C(0xc2b2ae3d27d4eb4f)
           + UINT64_C(0x165667b19e3779f9);
      p += 4;
      remaining -= 4;
   }
   while (remaining) {
      hash ^= *p++ * UINT64_C(0x27d4eb2f165667c5);
      hash = renku_xxh_rot64(hash, 11) * UINT64_C(0x9e3779b185ebca87);
      --remaining;
   }
   hash ^= hash >> 33; hash *= UINT64_C(0xc2b2ae3d27d4eb4f);
   hash ^= hash >> 29; hash *= UINT64_C(0x165667b19e3779f9);
   return hash ^ (hash >> 32);
}

#endif
