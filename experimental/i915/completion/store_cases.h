/* SPDX-License-Identifier: MIT */
#ifndef RENKU_STORE_CASES_H
#define RENKU_STORE_CASES_H
#include <native_pages.h>
#include <linux/highmem.h>
#ifndef RENKU_NATIVE
extern void renku_test_store_fail_pin(void);
extern void renku_test_store_fail_delete(void);
extern void renku_test_store_hold_pin(unsigned);
extern void renku_test_store_hold_unpin(unsigned);
extern unsigned renku_test_store_pin_entered(void);
extern unsigned renku_test_store_pinned_count(void);
#endif
struct store_fixture {
	struct renku_page_store *store;
	struct completion ready, gate, pinned, release, done;
	struct folio *seen[2];
	unsigned errors;
};
struct store_arg { struct store_fixture *fixture; unsigned index; };
static void StoreWorkerBody(struct store_arg *arg)
{
	struct store_fixture *f = arg->fixture;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(&f->ready); wait_for_completion(&f->gate);
	struct folio *anchor = renku_linux_page_store_read(f->store, 1);
	if (!anchor) Fatal();
	f->seen[arg->index] = anchor;
	complete(&f->pinned); wait_for_completion(&f->release);
	for (unsigned i = 0; i < 128; i++) {
		struct folio *folio = renku_linux_page_store_read(f->store, 1);
		if (!folio) Fatal();
		unsigned *word = kmap_local_folio(folio, arg->index * sizeof(unsigned));
		if (folio != anchor || (!i && *word != 0)) __atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		*word = i ^ 0x915;
		if (*word != (i ^ 0x915) || !preemptible() || pagefault_disabled())
			__atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		kunmap_local(word); folio_put(folio);
	}
	folio_put(anchor);
	renku_linux_task_unregister(&task); complete(&f->done);
}
#ifdef RENKU_NATIVE
static int32 StoreWorkerEntry(void *cookie) { StoreWorkerBody(cookie); return B_OK; }
static worker_id SpawnStoreWorker(struct store_arg *arg)
{
	thread_id id = spawn_kernel_thread(StoreWorkerEntry, "i915 store pin user", B_NORMAL_PRIORITY, arg);
	if (id < B_OK || resume_thread(id) != B_OK) Fatal();
	return id;
}
#else
static void *StoreWorkerEntry(void *cookie) { StoreWorkerBody(cookie); return NULL; }
static worker_id SpawnStoreWorker(struct store_arg *arg)
{
	pthread_t id; if (pthread_create(&id, NULL, StoreWorkerEntry, arg)) Fatal(); return id;
}
#endif
static void StoreCases(struct completion_report *r)
{
	CHECK(r, !renku_linux_page_store_create(0) && !renku_linux_page_store_create(PAGE_SIZE + 1));
	CHECK(r, !renku_linux_page_store_read(NULL, 0) && renku_linux_page_store_destroy(NULL) == -EINVAL);
#ifndef RENKU_NATIVE
	renku_test_page_fail_create(); CHECK(r, !renku_linux_page_store_create(5 * PAGE_SIZE));
#endif
	struct renku_page_store *store = renku_linux_page_store_create(5 * PAGE_SIZE);
	CHECK(r, store != NULL); if (!store) return;
	r->store_allocations++;
	CHECK(r, renku_linux_pages_uninit() == -EBUSY); /* empty stores also own backing */
	CHECK(r, !renku_linux_page_store_read(store, 5) && !renku_linux_page_store_read(store, ULONG_MAX));
	preempt_disable();
	CHECK(r, !renku_linux_page_store_create(PAGE_SIZE) && !renku_linux_page_store_read(store, 0));
	CHECK(r, renku_linux_page_store_destroy(store) == -EINVAL);
	preempt_enable();
#ifndef RENKU_NATIVE
	renku_test_store_fail_pin(); CHECK(r, !renku_linux_page_store_read(store, 2));
	CHECK(r, renku_test_store_pinned_count() == 0);
#endif
	struct folio *folio = renku_linux_page_store_read(store, 2);
	CHECK(r, folio != NULL); if (!folio) Fatal();
	CHECK(r, !folio_test_large(folio) && !PageCompound(&folio->page) && folio_ref_count(folio) == 1);
	CHECK(r, folio_nr_pages(folio) == 1 && folio_size(folio) == PAGE_SIZE);
	void *base = page_address(&folio->page);
#ifdef RENKU_NATIVE
	area_id area = area_for(base);
	area_info info;
	CHECK(r, area >= B_OK && get_area_info(area, &info) == B_OK && info.lock == B_NO_LOCK);
	CHECK(r, info.size == 5 * PAGE_SIZE && (char *)base == (char *)info.address + 2 * PAGE_SIZE);
	physical_entry entry; uint32 count = 1;
	CHECK(r, get_memory_map_etc(B_SYSTEM_TEAM, base, PAGE_SIZE, &entry, &count) == B_OK
		&& count == 1 && entry.size >= PAGE_SIZE && entry.address == page_to_phys(&folio->page));
#else
	uint64_t physical = page_to_phys(&folio->page);
	CHECK(r, renku_test_store_pinned_count() == 1);
#endif
	unsigned *data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE / sizeof(*data); i++) {
		CHECK(r, data[i] == 0); data[i] = i * 0x915 + 7;
	}
	kunmap_local(data);
	struct folio *same = renku_linux_page_store_read(store, 2);
	CHECK(r, same == folio && folio_ref_count(folio) == 2);
	CHECK(r, renku_linux_page_store_destroy(store) == -EBUSY);
	folio_put(same);
#ifndef RENKU_NATIVE
	renku_test_store_hold_unpin(1);
#endif
	preempt_disable(); folio_put(folio); preempt_enable();
#ifndef RENKU_NATIVE
	CHECK(r, renku_linux_pages_drain(2) == -ETIMEDOUT);
	CHECK(r, renku_linux_page_store_destroy(store) == -EBUSY && renku_test_store_pinned_count() == 1);
	renku_test_store_hold_unpin(0);
#endif
	CHECK(r, renku_linux_pages_drain(5000) == 0);
#ifndef RENKU_NATIVE
	CHECK(r, renku_test_store_pinned_count() == 0);
#endif
	folio = renku_linux_page_store_read(store, 2);
	CHECK(r, folio && page_address(&folio->page) == base); if (!folio) Fatal();
#ifndef RENKU_NATIVE
	CHECK(r, page_to_phys(&folio->page) != physical); /* modeled migration, no stale physical identity */
#endif
	data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE / sizeof(*data); i++) CHECK(r, data[i] == i * 0x915 + 7);
	kunmap_local(data); folio_put(folio);
	CHECK(r, renku_linux_pages_drain(5000) == 0);

	struct store_fixture f = {.store = store};
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.pinned);
	init_completion(&f.release); init_completion(&f.done);
	struct store_arg args[2] = {{&f, 0}, {&f, 1}};
	worker_id ids[2] = {SpawnStoreWorker(&args[0]), SpawnStoreWorker(&args[1])};
	CHECK(r, wait_for_completion_timeout(&f.ready, 2000) && wait_for_completion_timeout(&f.ready, 2000));
#ifndef RENKU_NATIVE
	unsigned before = renku_test_store_pin_entered(); renku_test_store_hold_pin(1);
#endif
	complete(&f.gate); complete(&f.gate);
#ifndef RENKU_NATIVE
	s64 deadline = NowUS() + 2000000;
	while (renku_test_store_pin_entered() < before + 2 && NowUS() < deadline) PauseUS(1000);
	CHECK(r, renku_test_store_pin_entered() == before + 2);
	CHECK(r, renku_linux_page_store_destroy(store) == -EBUSY);
	renku_test_store_hold_pin(0);
#endif
	CHECK(r, wait_for_completion_timeout(&f.pinned, 2000) && wait_for_completion_timeout(&f.pinned, 2000));
	CHECK(r, f.seen[0] && f.seen[0] == f.seen[1] && folio_ref_count(f.seen[0]) == 2);
#ifndef RENKU_NATIVE
	CHECK(r, renku_test_store_pinned_count() == 1); /* duplicate native pin was rolled back */
#endif
	CHECK(r, renku_linux_page_store_destroy(store) == -EBUSY);
	complete(&f.release); complete(&f.release);
	CHECK(r, wait_for_completion_timeout(&f.done, 2000) && wait_for_completion_timeout(&f.done, 2000));
	Join(ids[0]); Join(ids[1]); CHECK(r, f.errors == 0);
	CHECK(r, renku_linux_pages_drain(5000) == 0);
	r->store_workers = 2; r->store_iterations = 256;
	FinishFixture(&f.ready); FinishFixture(&f.gate); FinishFixture(&f.pinned);
	FinishFixture(&f.release); FinishFixture(&f.done);
#ifndef RENKU_NATIVE
	renku_test_store_fail_delete();
	CHECK(r, renku_linux_page_store_destroy(store) == -EIO && renku_linux_pages_uninit() == -EBUSY);
	folio = renku_linux_page_store_read(store, 2);
	CHECK(r, folio != NULL); if (!folio) Fatal();
	data = kmap_local_folio(folio, 0);
	CHECK(r, data[0] == 7 && data[PAGE_SIZE / sizeof(*data) - 1] == (PAGE_SIZE / sizeof(*data) - 1) * 0x915 + 7);
	kunmap_local(data); folio_put(folio);
	CHECK(r, renku_linux_pages_drain(5000) == 0);
#endif
	uintptr_t stale = (uintptr_t)store;
	CHECK(r, renku_linux_page_store_destroy(store) == 0); r->store_frees++;
	CHECK(r, !renku_linux_page_store_read((struct renku_page_store *)stale, 0)
		&& renku_linux_page_store_destroy((struct renku_page_store *)stale) == -EINVAL);
#ifdef RENKU_NATIVE
	status_t gone = get_area_info(area, &info);
	CHECK(r, gone == B_BAD_TEAM_ID || gone == B_BAD_VALUE);
#else
	CHECK(r, renku_test_store_pinned_count() == 0 && renku_test_page_live_count() == 0);
#endif
}
#endif
