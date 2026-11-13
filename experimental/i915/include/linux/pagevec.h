/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_PAGEVEC_H
#define RENKU_LINUX_PAGEVEC_H
#include <linux/mm.h>
/* Exceptional swap/DAX entries need their real cache provider. */
#define folio_batch_remove_exceptionals renku_linux_folio_batch_remove_exceptionals
#include "../../upstream/linux/pagevec.h"
#endif
