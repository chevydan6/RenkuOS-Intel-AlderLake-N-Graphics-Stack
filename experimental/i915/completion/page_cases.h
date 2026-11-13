/* SPDX-License-Identifier: MIT */
#ifndef RENKU_PAGE_CASES_H
#define RENKU_PAGE_CASES_H
#include <native_pages.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/scatterlist.h>
#include <linux/pagevec.h>
#include <linux/page-flags.h>
_Static_assert(__same_type(PAGE_SIZE, 1UL), "original folio shifts require unsigned-long page size");
#ifndef RENKU_NATIVE
extern unsigned renku_test_page_live_count(void);
extern unsigned renku_test_page_fault_depth(void);
extern void renku_test_page_fail_create(void);
extern void renku_test_page_hold_delete(unsigned);
#endif
static void PageCases(struct completion_report *r)
{
	CHECK(r, alloc_page(GFP_ATOMIC) == NULL);
	CHECK(r, alloc_pages(GFP_KERNEL, 1) == NULL);
	CHECK(r, alloc_page(GFP_HIGHUSER) == NULL);
	CHECK(r, alloc_pages(GFP_KERNEL | __GFP_COMP, 63) == NULL);
	struct page borrowed = {0};
	CHECK(r, !get_page_unless_zero(&borrowed) && nth_page(&borrowed, 0) == NULL);
#ifndef RENKU_NATIVE
	renku_test_page_fail_create(); CHECK(r, alloc_page(GFP_KERNEL) == NULL);
	CHECK(r, renku_test_page_live_count() == 0);
#endif
	for (unsigned order = 0; order < 4; order++) {
		struct page *head = alloc_pages(GFP_KERNEL | __GFP_ZERO | (order ? __GFP_COMP : 0), order);
		CHECK(r, head != NULL); if (!head) continue;
		r->page_allocations++;
		size_t count = (size_t)1 << order, size = count * PAGE_SIZE;
		void *base = page_address(head);
		uintptr_t descriptor_address = (uintptr_t)head;
		CHECK(r, !((uintptr_t)base & (size - 1)) && !(page_to_phys(head) & (size - 1)));
		CHECK(r, page_count(head) == 1 && compound_head(head) == head);
		struct folio *folio = page_folio(head);
		CHECK(r, folio && &folio->page == head && folio_ref_count(folio) == 1);
		CHECK(r, folio_order(folio) == order && folio_nr_pages(folio) == (long)count);
		CHECK(r, folio_size(folio) == size && folio_shift(folio) == PAGE_SHIFT + order);
		CHECK(r, folio_pfn(folio) == page_to_phys(head) >> PAGE_SHIFT);
		CHECK(r, PageHead(head) == (order != 0) && PageCompound(head) == (order != 0) && !PageTail(head));
		folio_get(folio); CHECK(r, page_count(head) == 2); folio_put(folio);
		CHECK(r, folio_try_get(folio) && folio_ref_count(folio) == 2); folio_put(folio);
		struct scatterlist sg = {0};
		sg_set_buf(&sg, (char *)base + 127, size - 127);
		CHECK(r, sg_page(&sg) == head && sg_virt(&sg) == (char *)base + 127);
		CHECK(r, sg_phys(&sg) == page_to_phys(head) + 127 && sg.length == size - 127);
		CHECK(r, renku_linux_pages_uninit() == -EBUSY);
#ifdef RENKU_NATIVE
		area_id area = area_for(base); area_info info;
		CHECK(r, area >= B_OK && get_area_info(area, &info) == B_OK && info.size == size);
		physical_entry physical; uint32 entries = 1;
		CHECK(r, get_memory_map_etc(B_SYSTEM_TEAM, base, size, &physical, &entries) == B_OK
			&& entries == 1 && physical.address == page_to_phys(head) && physical.size >= size);
#endif
		for (size_t i = 0; i < count; i++) {
			struct page *page = nth_page(head, i);
			CHECK(r, page && compound_head(page) == head && page_count(page) == 1);
			CHECK(r, page_address(page) == (char *)base + i * PAGE_SIZE);
			CHECK(r, page_to_phys(page) == page_to_phys(head) + i * PAGE_SIZE);
			CHECK(r, virt_to_page((char *)base + i * PAGE_SIZE + 127) == page);
			const struct page *read_only = page;
			_Static_assert(__same_type(page_folio(read_only), const struct folio *), "original const selection");
			CHECK(r, page_folio(page) == folio && page_folio(read_only) == folio);
			CHECK(r, folio_page(folio, i) == page && virt_to_folio((char *)base + i * PAGE_SIZE + 127) == folio);
			CHECK(r, PageTail(page) == (i != 0) && PageHead(page) == (order != 0 && i == 0));
			CHECK(r, compound_order(page) == (i ? 0 : order));
			CHECK(r, page_size(page) == (i ? PAGE_SIZE : size));
			CHECK(r, page_shift(page) == PAGE_SHIFT + (i ? 0 : order));
			if (i) CHECK(r, !folio_try_get((struct folio *)page));
			void *local = kmap_local_folio(folio, i * PAGE_SIZE + 127);
			CHECK(r, local == (char *)base + i * PAGE_SIZE + 127 && preemptible() && !pagefault_disabled());
			kunmap_local(local);
			CHECK(r, kmap_local_page(page) == page_address(page) && preemptible());
			kunmap_local(page_address(page));
			unsigned *words = kmap_atomic(page);
			CHECK(r, words == page_address(page) && pagefault_disabled() && preempt_count() == 1);
#ifndef RENKU_NATIVE
			CHECK(r, renku_test_page_fault_depth() == 1);
#endif
			for (unsigned j = 0; j < PAGE_SIZE / sizeof(*words); j++)
				if (words[j]) { CHECK(r, false); break; }
			words[0] = (unsigned)i + 915;
			CHECK(r, words[0] == (unsigned)i + 915);
			void *nested = kmap_atomic(page);
			CHECK(r, nested == words && preempt_count() == 2 && current->pagefault_disabled == 2);
			kunmap_atomic((char *)nested + 7);
			CHECK(r, preempt_count() == 1 && pagefault_disabled());
			kunmap_atomic(words);
			CHECK(r, !pagefault_disabled() && preemptible());
		}
		CHECK(r, nth_page(head, count) == NULL);
		struct page *last = nth_page(head, count - 1);
		get_page(last); CHECK(r, page_count(head) == 2 && page_count(last) == 2);
		put_page(last); CHECK(r, page_count(head) == 1);
#ifndef RENKU_NATIVE
		renku_test_page_hold_delete(1);
#endif
		preempt_disable();
		CHECK(r, alloc_page(GFP_KERNEL) == NULL);
		__free_pages(head, order); /* queues actual sleeping native release */
		CHECK(r, virt_to_page(base) == NULL && !get_page_unless_zero(head));
		preempt_enable();
#ifndef RENKU_NATIVE
		CHECK(r, renku_linux_pages_drain(2) == -ETIMEDOUT && renku_linux_pages_uninit() == -EBUSY);
		renku_test_page_hold_delete(0);
#endif
		CHECK(r, renku_linux_pages_drain(5000) == 0);
#ifdef RENKU_NATIVE
		status_t gone = get_area_info(area, &info);
		CHECK(r, gone == B_BAD_TEAM_ID || gone == B_BAD_VALUE);
#else
		CHECK(r, renku_test_page_live_count() == 0 && renku_test_page_fault_depth() == 0);
#endif
		CHECK(r, !get_page_unless_zero((struct page *)descriptor_address));
		r->page_frees++;
	}
}
struct page_fixture { struct page *head; struct completion ready, gate, done; unsigned errors; };
struct page_arg { struct page_fixture *f; unsigned index; };
static void PageWorkerBody(struct page_arg *arg)
{
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	struct page_fixture *f = arg->f;
	complete(&f->ready); wait_for_completion(&f->gate);
	for (unsigned i = 0; i < 256; i++) {
		if (!get_page_unless_zero(f->head)) Fatal();
		struct page *page = nth_page(f->head, arg->index);
		struct folio *folio = page_folio(page);
		if (i & 1) {
			if (!folio_try_get(folio)) Fatal();
			unsigned *word = kmap_local_folio(folio, arg->index * PAGE_SIZE); *word = i;
			if (*word != i || !preemptible() || pagefault_disabled())
				__atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
			kunmap_local(word); folio_put(folio);
		} else {
			unsigned *word = kmap_atomic(page); *word = i;
			if (*word != i) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
			kunmap_atomic(word);
		}
		put_page(page);
	}
	if (pagefault_disabled() || !preemptible()) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
	renku_linux_task_unregister(&task); complete(&f->done);
}
#ifdef RENKU_NATIVE
static status_t PageWorker(void *cookie) { PageWorkerBody(cookie); return B_OK; }
#else
static void *PageWorker(void *cookie) { PageWorkerBody(cookie); return NULL; }
#endif
static void ConcurrentPageCases(struct completion_report *r)
{
	struct page_fixture f = {.head = alloc_pages(GFP_KERNEL | __GFP_COMP, 1)};
	CHECK(r, f.head != NULL); if (!f.head) return;
	r->page_allocations++;
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.done);
	struct page_arg args[] = {{&f, 0}, {&f, 1}};
	worker_id ids[2];
	for (unsigned i = 0; i < 2; i++) {
#ifdef RENKU_NATIVE
		ids[i] = spawn_kernel_thread(PageWorker, "i915 owned page user", B_NORMAL_PRIORITY, &args[i]);
		if (ids[i] < B_OK || resume_thread(ids[i]) != B_OK) Fatal();
#else
		if (pthread_create(&ids[i], NULL, PageWorker, &args[i])) Fatal();
#endif
	}
	for (unsigned i = 0; i < 2; i++) if (!wait_for_completion_timeout(&f.ready, 2000)) Fatal();
	if (!UntilQueued(&f.gate, 2)) Fatal();
	complete_all(&f.gate);
	for (unsigned i = 0; i < 2; i++) { if (!wait_for_completion_timeout(&f.done, 2000)) Fatal(); Join(ids[i]); }
	CHECK(r, f.errors == 0 && page_count(f.head) == 1);
	put_page(f.head);
	CHECK(r, renku_linux_pages_drain(5000) == 0);
	r->page_frees++; r->page_workers = 2; r->page_iterations = 512;
	FinishFixture(&f.ready); FinishFixture(&f.gate); FinishFixture(&f.done);
}

static void FolioBatchCases(struct completion_report *r)
{
	struct folio *first = folio_alloc(GFP_KERNEL | __GFP_ZERO, 0);
	struct folio *second = folio_alloc(GFP_KERNEL | __GFP_ZERO, 2);
	CHECK(r, first && second);
	if (!first || !second) { if (first) folio_put(first); if (second) folio_put(second); return; }
	r->page_allocations += 2;
	CHECK(r, !folio_test_large(first) && folio_test_large(second));
	CHECK(r, !PageHead(&first->page) && !PageTail(&first->page) && !PageCompound(&first->page));
	CHECK(r, PageHead(&second->page) && !PageTail(&second->page) && PageCompound(&second->page));
#ifdef RENKU_NATIVE
	area_id areas[] = {area_for(page_address(&first->page)), area_for(page_address(&second->page))};
#endif
	uintptr_t stale = (uintptr_t)second;
	struct folio_batch batch;
	folio_batch_init(&batch);
	CHECK(r, !folio_batch_count(&batch) && folio_batch_space(&batch) == PAGEVEC_SIZE && !batch.percpu_pvec_drained);
	for (unsigned i = 0; i < PAGEVEC_SIZE; i++) {
		struct folio *folio = i & 1 ? first : second;
		folio_get(folio);
		CHECK(r, folio_batch_add(&batch, folio) == PAGEVEC_SIZE - i - 1);
	}
	CHECK(r, folio_ref_count(first) == 8 && folio_ref_count(second) == 9);
	folio_batch_release(&batch);
	CHECK(r, !folio_batch_count(&batch) && batch.percpu_pvec_drained);
	CHECK(r, folio_ref_count(first) == 1 && folio_ref_count(second) == 1);
	folio_batch_release(&batch); /* empty batch consumes no reference */
	folio_batch_add(&batch, first); folio_batch_add(&batch, second);
	preempt_disable();
	folio_batch_release(&batch); /* release actual final references without sleeping */
	CHECK(r, !folio_batch_count(&batch) && !folio_try_get((struct folio *)stale));
	preempt_enable();
	CHECK(r, renku_linux_pages_drain(5000) == 0);
#ifdef RENKU_NATIVE
	for (unsigned i = 0; i < 2; i++) {
		area_info info; status_t gone = get_area_info(areas[i], &info);
		CHECK(r, gone == B_BAD_TEAM_ID || gone == B_BAD_VALUE);
	}
#else
	CHECK(r, renku_test_page_live_count() == 0);
#endif
	r->page_frees += 2;
}

#endif
