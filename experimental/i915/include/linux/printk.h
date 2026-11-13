/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_PRINTK_H
#define RENKU_LINUX_PRINTK_H
/* dma-fence.h includes printk but calls no printk API. Actual warnings are
 * provided by kpi_base; unsupported printk entry points remain undeclared. */
#include <kpi_base.h>
struct seq_file;
#endif
