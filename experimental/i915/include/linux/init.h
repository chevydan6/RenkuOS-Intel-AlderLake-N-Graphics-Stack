/* SPDX-License-Identifier: MIT */
#ifndef RENKU_INIT_ANNOTATIONS_H
#define RENKU_INIT_ANNOTATIONS_H
/* Native module code stays resident until unload; no Linux init section discard. */
#define __init __attribute__((cold))
#define __exit __attribute__((cold))
#endif
