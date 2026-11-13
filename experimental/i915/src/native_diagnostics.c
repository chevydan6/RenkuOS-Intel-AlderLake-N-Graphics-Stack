/* SPDX-License-Identifier: MIT */
#include <kpi_base.h>
#ifndef RENKU_NATIVE
#error This adapter requires real RenkuOS kernel interfaces
#endif
bool renku_kpi_warn(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	dvprintf(format, args);
	va_end(args);
	return true;
}
