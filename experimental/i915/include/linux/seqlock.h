/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_SEQLOCK_H
#define RENKU_LINUX_SEQLOCK_H
/* v6.6 dma-resv.h includes this header but calls no seqcount API. No sequence
 * lock operations or always-successful retry checks are provided. */
#include <kpi_base.h>
#endif
