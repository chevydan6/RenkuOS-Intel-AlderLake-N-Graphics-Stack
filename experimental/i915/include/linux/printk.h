/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_PRINTK_H
#define RENKU_LINUX_PRINTK_H
#include <kpi_base.h>
#include <linux/kern_levels.h>
struct seq_file;
struct va_format; /* Linux %pV/devices need their own formatting/entry adapters. */
/* Bounded common-C format bridge. Pointer extensions, floating point and %n
 * return -EINVAL and emit an explicit diagnostic. Truncation returns -ENOSPC. */
int renku_linux_printk(const char *, ...) __printf(1, 2);
void renku_linux_log_emit(char level, const char *text);
#define printk renku_linux_printk
#define pr_warn(fmt, ...) printk(KERN_WARNING fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...) printk(KERN_ERR fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...) printk(KERN_NOTICE fmt, ##__VA_ARGS__)
/* Device printk entry points remain unresolved; CONFIG_PRINTK selects their
 * real declarations rather than Linux's disabled-logging no-op methods. */
#endif
