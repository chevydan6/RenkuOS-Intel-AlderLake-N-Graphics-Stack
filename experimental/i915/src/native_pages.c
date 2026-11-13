/* SPDX-License-Identifier: MIT */
#include <native_pages.h>
#include <native_page_bridge.h>
#include <linux/gfp.h>
#include <linux/preempt.h>
#include <linux/refcount.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/compiler.h>
struct page_run {
	struct page *pages;
	void *base;
	size_t count, size;
	unsigned order;
	int32_t area;
	bool compound;
	refcount_t references;
	struct page_run *next;
};
static DEFINE_SPINLOCK(sLock);
static struct page_run *sRuns, *sQueue, **sQueueTail = &sQueue;
static struct renku_page_event *sEvent;
static struct renku_page_worker *sWorker;
static unsigned sPending;
static bool sReady, sStop;
static void Invalid(const char *why) { panic("i915 pages: %s", why); __builtin_trap(); }
static struct page_run *Find(const struct page *page, size_t *index)
{
	uintptr_t address = (uintptr_t)page;
	for (struct page_run *run = sRuns; run; run = run->next) {
		uintptr_t base = (uintptr_t)run->pages;
		if (address >= base && address - base < run->count * sizeof(*page)
			&& (address - base) % sizeof(*page) == 0) {
			*index = (address - base) / sizeof(*page); return run;
		}
	}
	return NULL;
}
static int32_t Releaser(void *unused)
{
	(void)unused;
	for (;;) {
		renku_vm_page_event_wait(sEvent);
		spin_lock(&sLock);
		struct page_run *run = sQueue;
		if (run) {
			sQueue = run->next;
			if (!sQueue) sQueueTail = &sQueue;
		}
		bool stop = sStop;
		spin_unlock(&sLock);
		if (!run) { if (stop) return 0; else continue; }
		if (renku_vm_page_run_delete(run->area) != 0) Invalid("native area deletion failed");
		free(run->pages); free(run);
		spin_lock(&sLock); sPending--; spin_unlock(&sLock);
	}
}
int renku_linux_pages_init(void)
{
	if (sReady || sWorker || !preemptible()) return -EBUSY;
	sRuns = sQueue = NULL; sQueueTail = &sQueue; sPending = 0; sStop = false;
	sEvent = renku_vm_page_event_create();
	if (!sEvent) return -ENOMEM;
	sWorker = renku_vm_page_worker_start(Releaser, NULL);
	if (!sWorker) { renku_vm_page_event_delete(sEvent); sEvent = NULL; return -ENOMEM; }
	sReady = true; return 0;
}
int renku_linux_pages_drain(unsigned int timeout_ms)
{
	if (!sReady || !preemptible()) return -EINVAL;
	int64_t deadline = renku_vm_page_now() + (int64_t)timeout_ms * 1000;
	for (;;) {
		spin_lock(&sLock); unsigned pending = sPending; spin_unlock(&sLock);
		if (!pending) return 0;
		if (renku_vm_page_now() >= deadline) return -ETIMEDOUT;
		renku_vm_page_pause();
	}
}
int renku_linux_pages_uninit(void)
{
	if (!sReady || !preemptible()) return -EINVAL;
	spin_lock(&sLock);
	if (sRuns || sPending) { spin_unlock(&sLock); return -EBUSY; }
	sStop = true; spin_unlock(&sLock);
	renku_vm_page_event_signal(sEvent);
	renku_vm_page_worker_join(sWorker); sWorker = NULL;
	renku_vm_page_event_delete(sEvent); sEvent = NULL;
	sReady = false; return 0;
}
struct page *alloc_pages(gfp_t flags, unsigned int order)
{
	/* Higher-order runs need compound lifetime. Splitting ordinary higher-order
	 * pages, movable/highmem/zone allocation and atomic reserves are separate. */
	if (!sReady || !preemptible() || !gfpflags_allow_blocking(flags)
		|| (flags & ~(GFP_KERNEL | __GFP_ZERO | __GFP_COMP))
		|| (order && !(flags & __GFP_COMP)) || order >= sizeof(size_t) * CHAR_BIT - PAGE_SHIFT)
		return NULL;
	size_t count = (size_t)1 << order, size = count << PAGE_SHIFT;
	if (count > SIZE_MAX / sizeof(struct page)) return NULL;
	struct page_run *run = calloc(1, sizeof(*run));
	if (!run) return NULL;
	run->pages = calloc(count, sizeof(*run->pages));
	if (!run->pages) { free(run); return NULL; }
	uint64_t physical;
	run->area = renku_vm_page_run_create(size, &run->base, &physical);
	if (run->area < 0) { free(run->pages); free(run); return NULL; }
	if (flags & __GFP_ZERO) memset(run->base, 0, size);
	run->count = count; run->size = size; run->order = order;
	run->compound = (flags & __GFP_COMP) != 0;
	refcount_set(&run->references, 1);
	for (size_t i = 0; i < count; i++) {
		run->pages[i].virtual = (char *)run->base + (i << PAGE_SHIFT);
		run->pages[i].physical = physical + (i << PAGE_SHIFT);
	}
	spin_lock(&sLock); run->next = sRuns; sRuns = run; spin_unlock(&sLock);
	return run->pages;
}
struct page *virt_to_page(const void *address)
{
	uintptr_t value = (uintptr_t)address;
	struct page *page = NULL;
	spin_lock(&sLock);
	for (struct page_run *run = sRuns; run; run = run->next) {
		uintptr_t base = (uintptr_t)run->base;
		if (value >= base && value - base < run->size) {
			page = &run->pages[(value - base) >> PAGE_SHIFT]; break;
		}
	}
	spin_unlock(&sLock); return page; /* borrowed; caller must own the backing */
}
struct page *nth_page(struct page *page, unsigned long n)
{
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	struct page *result = run && n < run->count - index ? &run->pages[index + n] : NULL;
	spin_unlock(&sLock); return result;
}
struct page *compound_head(struct page *page)
{
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	struct page *head = run ? (run->compound ? run->pages : page) : NULL;
	spin_unlock(&sLock); return head;
}
bool get_page_unless_zero(struct page *page)
{
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	bool got = run && (!index || run->compound) && refcount_inc_not_zero(&run->references);
	spin_unlock(&sLock); return got;
}
void get_page(struct page *page) { if (!get_page_unless_zero(page)) Invalid("get of unowned page"); }
unsigned int page_count(const struct page *page)
{
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	unsigned count = run && (!index || run->compound) ? refcount_read(&run->references) : 0;
	spin_unlock(&sLock); return count;
}
void put_page(struct page *page)
{
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	if (!run || (index && !run->compound)) Invalid("put of unowned page");
	bool last = refcount_dec_and_test(&run->references);
	if (last) {
		struct page_run **slot = &sRuns;
		while (*slot != run) slot = &(*slot)->next;
		*slot = run->next;
		run->next = NULL; *sQueueTail = run; sQueueTail = &run->next; sPending++;
	}
	spin_unlock(&sLock);
	if (last) renku_vm_page_event_signal(sEvent);
}
void __free_pages(struct page *page, unsigned int order)
{
	if (!page) return;
	spin_lock(&sLock);
	size_t index; struct page_run *run = Find(page, &index);
	if (!run || index || run->order != order) Invalid("free page order/head mismatch");
	spin_unlock(&sLock); put_page(page);
}
void renku_linux_pagefault_disable(void)
{
	struct task_struct *task = current;
	if (task->pagefault_disabled == UINT_MAX) Invalid("fault nesting overflow");
	task->pagefault_disabled++; renku_vm_pagefault_forbid(); barrier();
}
void renku_linux_pagefault_enable(void)
{
	struct task_struct *task = current;
	if (!task->pagefault_disabled) Invalid("unmatched fault enable");
	barrier(); renku_vm_pagefault_permit(); task->pagefault_disabled--;
}
bool renku_linux_pagefault_disabled(void) { return current->pagefault_disabled != 0; }
