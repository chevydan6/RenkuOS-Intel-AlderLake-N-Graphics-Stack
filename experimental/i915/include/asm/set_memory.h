/* SPDX-License-Identifier: MIT */
#ifndef RENKU_ASM_SET_MEMORY_H
#define RENKU_ASM_SET_MEMORY_H
/* Cache-attribute changes require native mappings. These declarations do not
 * make unsupported attribute changes succeed. */
int set_memory_uc(unsigned long, int);
int set_memory_wc(unsigned long, int);
int set_memory_wb(unsigned long, int);
#endif
