/* SPDX-License-Identifier: MIT */
#ifndef RENKU_I915_NATIVE_SERVICES_H
#define RENKU_I915_NATIVE_SERVICES_H
#include <kpi_base.h>
#ifndef RENKU_NATIVE
#error These services require real RenkuOS kernel facilities
#endif
/* CPU physical backing only. No device DMA address is established here. */
struct renku_kpi_page {
	area_id area;
	void *cpu;
	phys_addr_t physical;
	bool physical_valid;
};
status_t renku_kpi_page_create(struct renku_kpi_page *);
status_t renku_kpi_page_destroy(struct renku_kpi_page *);
/* Explicit native resource lifecycle; not a complete Linux completion API. */
struct renku_kpi_completion { sem_id semaphore; };
status_t renku_kpi_completion_init(struct renku_kpi_completion *);
status_t renku_kpi_completion_destroy(struct renku_kpi_completion *);
status_t renku_kpi_completion_signal(struct renku_kpi_completion *);
status_t renku_kpi_completion_wait(struct renku_kpi_completion *, bigtime_t timeout_us);
#endif
