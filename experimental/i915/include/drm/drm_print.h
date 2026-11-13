/* SPDX-License-Identifier: MIT */
#ifndef RENKU_DRM_PRINT_H
#define RENKU_DRM_PRINT_H
#include <kpi_base.h>
struct drm_printer {
	void (*print)(void *arg, const char *format, va_list args);
	void *arg;
};
static inline void drm_printf(struct drm_printer *p, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	p->print(p->arg, format, args);
	va_end(args);
}
#endif
