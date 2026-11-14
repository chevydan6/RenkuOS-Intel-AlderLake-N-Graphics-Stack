/* SPDX-License-Identifier: MIT */
/* Host integration tests: actual owner/refcount code and real RAM/threads.
 * The native VM boundary is the existing explicit host model, not GPU evidence. */
#include <native_pages.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/shmem_fs.h>
#include <assert.h>
#include <stdio.h>
extern unsigned renku_test_page_live_count(void);
extern unsigned renku_test_store_pinned_count(void);
extern void renku_test_store_fail_delete(void);
extern void renku_test_store_hold_unpin(unsigned);
extern unsigned renku_test_store_noreserve_count(void);
extern void renku_test_page_fail_create(void);
extern void renku_test_store_hold_pin(unsigned);
extern unsigned renku_test_store_pin_entered(void);

struct pin_attempt { struct renku_page_store *store; struct folio *folio; };
static void *PinWhileClosing(void *cookie)
{
	struct pin_attempt *attempt = cookie;
	attempt->folio = renku_linux_page_store_read(attempt->store, 0);
	return NULL;
}

static unsigned released;
static void Released(void *cookie)
{
	assert(cookie == &released && preemptible());
	__atomic_fetch_add(&released, 1, __ATOMIC_RELEASE);
}
static void StoreOwners(void)
{
	struct renku_page_store *store = renku_linux_page_store_create_owned(PAGE_SIZE, false, Released, &released);
	assert(store && renku_linux_page_store_get(store));
	assert(renku_linux_page_store_destroy(store) == -EBUSY);
	renku_linux_page_store_put(store);
	struct folio *folio = renku_linux_page_store_read(store, 0);
	assert(folio && renku_test_store_pinned_count() == 1);
	unsigned *word = kmap_local_folio(folio, 0);
	assert(*word == 0); *word = 0x915cafe; kunmap_local(word);
	renku_linux_page_store_put(store);
	assert(!renku_linux_page_store_get(store));
	assert(!renku_linux_page_store_read(store, 0));
	assert(renku_linux_pages_drain(2) == -ETIMEDOUT);
	assert(renku_linux_pages_uninit() == -EBUSY);
	assert(renku_test_page_live_count() == 1 && !__atomic_load_n(&released, __ATOMIC_ACQUIRE));
	word = kmap_local_folio(folio, 0); assert(*word == 0x915cafe); kunmap_local(word);
	renku_test_store_hold_unpin(1);
	folio_put(folio);
	assert(renku_linux_pages_drain(2) == -ETIMEDOUT);
	assert(renku_test_page_live_count() == 1 && !__atomic_load_n(&released, __ATOMIC_ACQUIRE));
	renku_test_store_hold_unpin(0);
	assert(renku_linux_pages_drain(5000) == 0);
	assert(renku_test_page_live_count() == 0 && renku_test_store_pinned_count() == 0);
	assert(__atomic_load_n(&released, __ATOMIC_ACQUIRE) == 1);

	store = renku_linux_page_store_create_owned(PAGE_SIZE, false, Released, &released);
	assert(store); renku_test_store_fail_delete();
	renku_linux_page_store_put(store);
	assert(renku_linux_pages_drain(5000) == 0);
	assert(renku_test_page_live_count() == 0 && __atomic_load_n(&released, __ATOMIC_ACQUIRE) == 2);

	store = renku_linux_page_store_create_owned(PAGE_SIZE, false, Released, &released);
	assert(store); unsigned before = renku_test_store_pin_entered();
	renku_test_store_hold_pin(1);
	struct pin_attempt attempt = {.store = store}; pthread_t thread;
	assert(!pthread_create(&thread, NULL, PinWhileClosing, &attempt));
	for (unsigned i = 0; renku_test_store_pin_entered() == before && i < 1000; i++)
		usleep(1000);
	assert(renku_test_store_pin_entered() == before + 1);
	renku_linux_page_store_put(store);
	assert(renku_linux_pages_drain(2) == -ETIMEDOUT);
	assert(renku_test_page_live_count() == 1 && __atomic_load_n(&released, __ATOMIC_ACQUIRE) == 2);
	renku_test_store_hold_pin(0); assert(!pthread_join(thread, NULL));
	assert(attempt.folio && renku_test_store_pinned_count() == 1);
	folio_put(attempt.folio); assert(renku_linux_pages_drain(5000) == 0);
	assert(renku_test_page_live_count() == 0 && __atomic_load_n(&released, __ATOMIC_ACQUIRE) == 3);
}
struct file_reader { struct file *file; struct folio *folio; unsigned lane; pthread_barrier_t *barrier; };
static void *ReadSharedFile(void *cookie)
{
	struct file_reader *reader = cookie;
	reader->folio = shmem_read_folio_gfp(reader->file->f_mapping, 0, GFP_KERNEL);
	assert(!IS_ERR(reader->folio));
	int status = pthread_barrier_wait(reader->barrier);
	assert(status == 0 || status == PTHREAD_BARRIER_SERIAL_THREAD);
	for (unsigned i = 0; i < 256; i++) {
		struct page *page = shmem_read_mapping_page(reader->file->f_mapping, 0);
		assert(!IS_ERR(page) && page == &reader->folio->page);
		unsigned *word = (unsigned *)page_address(page) + reader->lane;
		if (i) assert(*word == (0x915000U + i - 1));
		else assert(*word == 0);
		*word = 0x915000U + i;
		put_page(page);
	}
	folio_put(reader->folio); fput(reader->file);
	return NULL;
}
/* A missing file reference or early final-close deletion must corrupt/fail
 * this real-RAM test; changing source text or a mock response cannot pass it. */
static void ShmemFiles(void)
{
	assert(PTR_ERR(shmem_file_setup(NULL, PAGE_SIZE, 0)) == -EINVAL);
	assert(PTR_ERR(shmem_file_setup("negative", -1, 0)) == -EINVAL);
	assert(PTR_ERR(shmem_file_setup("oversized", LLONG_MAX, 0)) == -EFBIG);
	assert(PTR_ERR(shmem_file_setup("unknown flags", PAGE_SIZE, 1)) == -EINVAL);
	renku_test_page_fail_create();
	assert(PTR_ERR(shmem_file_setup("failed allocation", PAGE_SIZE, 0)) == -ENOMEM);
	assert(renku_test_page_live_count() == 0);
	struct file *empty = shmem_file_setup("empty", 0, 0);
	assert(!IS_ERR(empty) && i_size_read(file_inode(empty)) == 0);
	assert(PTR_ERR(shmem_read_mapping_page(empty->f_mapping, 0)) == -EINVAL);
	fput(empty); assert(renku_linux_pages_drain(5000) == 0);

	struct file *file = shmem_file_setup("i915", PAGE_SIZE + 19, VM_NORESERVE);
	assert(!IS_ERR(file) && file_count(file) == 1);
	assert(renku_test_store_noreserve_count() == 1);
	struct address_space *mapping = file->f_mapping;
	assert(mapping->host == file_inode(file) && file_inode(file)->i_mapping == mapping);
	assert(i_size_read(mapping->host) == PAGE_SIZE + 19);
	assert(get_file(file) == file && file_count(file) == 2);
	fput(file); assert(file_count(file) == 1);
	assert(mapping_gfp_mask(mapping) == GFP_KERNEL);
	mapping_set_gfp_mask(mapping, GFP_KERNEL | __GFP_NOWARN);
	assert(mapping_gfp_constraint(mapping, ~__GFP_RECLAIM) == (__GFP_IO | __GFP_FS | __GFP_NOWARN));
	assert(PTR_ERR(shmem_read_mapping_page(mapping, 0)) == -EOPNOTSUPP);
	mapping_set_gfp_mask(mapping, GFP_KERNEL);
	assert(PTR_ERR(shmem_read_folio_gfp(mapping, 0, GFP_ATOMIC)) == -EOPNOTSUPP);
	assert(PTR_ERR(shmem_read_folio_gfp(mapping, 2, GFP_KERNEL)) == -EINVAL);
	assert(PTR_ERR(shmem_read_folio_gfp(mapping, ULONG_MAX, GFP_KERNEL)) == -EINVAL);
	struct folio *anchor = shmem_read_folio_gfp(mapping, 0, GFP_KERNEL);
	assert(!IS_ERR(anchor));
	pthread_barrier_t barrier; assert(!pthread_barrier_init(&barrier, NULL, 2));
	struct file_reader readers[2] = {{get_file(file), NULL, 0, &barrier}, {get_file(file), NULL, 1, &barrier}};
	pthread_t threads[2];
	for (unsigned i = 0; i < 2; i++) assert(!pthread_create(&threads[i], NULL, ReadSharedFile, &readers[i]));
	for (unsigned i = 0; i < 2; i++) assert(!pthread_join(threads[i], NULL));
	assert(readers[0].folio == anchor && readers[1].folio == anchor && file_count(file) == 1);
	assert(!pthread_barrier_destroy(&barrier));
	folio_put(anchor);
	assert(renku_linux_pages_drain(5000) == 0);
	struct folio *folio = shmem_read_folio_gfp(mapping, 1, GFP_KERNEL);
	assert(!IS_ERR(folio));
	unsigned char *data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE; i++) { assert(data[i] == 0); data[i] = (unsigned char)(i ^ 0x5a); }
	kunmap_local(data);
	struct page *same = shmem_read_mapping_page(mapping, 1);
	assert(same == &folio->page && folio_ref_count(folio) == 2);
	put_page(same); folio_put(folio);
	assert(renku_linux_pages_drain(5000) == 0 && renku_test_store_pinned_count() == 0);
	folio = shmem_read_folio_gfp(mapping, 1, GFP_KERNEL);
	assert(!IS_ERR(folio)); data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE; i++) assert(data[i] == (unsigned char)(i ^ 0x5a));
	kunmap_local(data);
	fput(file);
	assert(renku_linux_pages_drain(2) == -ETIMEDOUT);
	assert(i_size_read(mapping->host) == PAGE_SIZE + 19 && renku_test_page_live_count() == 1);
	data = kmap_local_folio(folio, 0); assert(data[0] == 0x5a && data[19] == 0x49); kunmap_local(data);
	folio_put(folio); assert(renku_linux_pages_drain(5000) == 0);
	assert(renku_test_page_live_count() == 0 && renku_test_store_pinned_count() == 0);
}
int main(void)
{
	struct task_struct task;
	assert(renku_linux_task_register(&task));
	assert(renku_linux_pages_init() == 0);
	StoreOwners();
	ShmemFiles();
	assert(renku_linux_pages_uninit() == 0);
	renku_linux_task_unregister(&task);
	puts("PASS owned backing survives final close, queued unpin and delete failure; callbacks retire once");
	return 0;
}
