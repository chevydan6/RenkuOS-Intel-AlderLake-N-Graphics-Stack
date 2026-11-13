/* SPDX-License-Identifier: MIT */
#include <linux/printk.h>
#include <linux/errno.h>
/* The native declaration and exported signature match POSIX vsnprintf. Avoid
 * including host stdio while compiling native KernelExport's dprintf namespace. */
extern int vsnprintf(char *, size_t, const char *, va_list);
static bool Supported(const char *format)
{
	for (const char *p = format; *p; p++) {
		if (*p != '%') continue;
		p++;
		if (*p == '%') continue;
		while (*p && strchr("#0-+ ", *p)) p++;
		if (*p == '*') p++; else while (*p >= '0' && *p <= '9') p++;
		if (*p == '.') {
			p++;
			if (*p == '*') p++; else while (*p >= '0' && *p <= '9') p++;
		}
		bool has_length = false;
		if (*p == 'h' || *p == 'l') { has_length = true; char length = *p++; if (*p == length) p++; }
		else if (*p && strchr("jzt", *p)) { has_length = true; p++; }
		if (!*p || !strchr("diuoxXcsp", *p)) return false;
		if (has_length && strchr("csp", *p)) return false;
		if (*p == 'p' && ((p[1] >= 'a' && p[1] <= 'z') || (p[1] >= 'A' && p[1] <= 'Z')))
			return false;
	}
	return true;
}
int renku_linux_printk(const char *format, ...)
{
	if (!format) return -EINVAL;
	char level = 'd';
	if (format[0] == KERN_SOH_ASCII) {
		if (format[1] < '0' || format[1] > '7') {
			renku_linux_log_emit('3', "unsupported Linux log prefix\n");
			return -EINVAL;
		}
		level = format[1]; format += 2;
	}
	if (!Supported(format)) {
		renku_linux_log_emit('3', "unsupported Linux log format: ");
		renku_linux_log_emit('3', format); /* literal text, never a format string */
		return -EINVAL;
	}
	char text[512];
	va_list args;
	va_start(args, format);
	int count = vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	if (count < 0) return -EINVAL;
	renku_linux_log_emit(level, text);
	if ((size_t)count >= sizeof(text)) {
		renku_linux_log_emit('3', "[previous log truncated]\n");
		return -ENOSPC;
	}
	return count;
}
