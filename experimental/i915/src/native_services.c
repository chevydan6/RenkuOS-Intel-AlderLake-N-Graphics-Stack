/* SPDX-License-Identifier: MIT */
#include <native_services.h>

status_t renku_kpi_page_create(struct renku_kpi_page *page)
{
	*page = (struct renku_kpi_page){ .area = -1 };
	if (B_PAGE_SIZE != 4096) return B_NOT_SUPPORTED;
	page->area = create_area("i915 kpi wired page", &page->cpu,
		B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (page->area < B_OK) return page->area;
	physical_entry entry;
	uint32 count = 1;
	status_t error = get_memory_map_etc(B_CURRENT_TEAM, page->cpu,
		B_PAGE_SIZE, &entry, &count);
	if (error != B_OK) return error;
	if (count != 1 || entry.size < B_PAGE_SIZE || (entry.address & 4095))
		return B_BAD_ADDRESS;
	page->physical = entry.address;
	page->physical_valid = true;
	return B_OK;
}
status_t renku_kpi_page_destroy(struct renku_kpi_page *page)
{
	if (page->area < 0) return B_BAD_VALUE;
	status_t error = delete_area(page->area);
	/* Preserve the owning record if native deletion fails. No GPU publication
 * or asynchronous user of this backing is supported by this CPU backend. */
	if (error == B_OK) *page = (struct renku_kpi_page){ .area = -1 };
	return error;
}
status_t renku_kpi_completion_init(struct renku_kpi_completion *c)
{
	c->semaphore = create_sem(0, "i915 kpi completion");
	return c->semaphore < 0 ? c->semaphore : B_OK;
}
status_t renku_kpi_completion_destroy(struct renku_kpi_completion *c)
{
	status_t error = delete_sem(c->semaphore);
	if (error == B_OK) c->semaphore = -1;
	return error;
}
status_t renku_kpi_completion_signal(struct renku_kpi_completion *c)
{
	return release_sem_etc(c->semaphore, 1, B_DO_NOT_RESCHEDULE);
}
status_t renku_kpi_completion_wait(struct renku_kpi_completion *c, bigtime_t timeout_us)
{
	bigtime_t now = system_time();
	if (timeout_us < 0 || timeout_us > INT64_MAX - now) return B_BAD_VALUE;
	bigtime_t deadline = now + timeout_us;
	status_t error;
	do {
		error = acquire_sem_etc(c->semaphore, 1, B_ABSOLUTE_TIMEOUT, deadline);
	} while (error == B_INTERRUPTED);
	return error;
}
