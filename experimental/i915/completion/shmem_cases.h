/* SPDX-License-Identifier: MIT */
#ifndef RENKU_SHMEM_CASES_H
#define RENKU_SHMEM_CASES_H
#include <linux/shmem_fs.h>
struct shmem_fixture {
	struct file *file;
	struct completion ready, gate, pinned, release, done;
	struct folio *seen[2];
	unsigned errors;
};
struct shmem_arg { struct shmem_fixture *fixture; unsigned lane; };
static void ShmemWorkerBody(struct shmem_arg *arg)
{
	struct shmem_fixture *f = arg->fixture;
	struct task_struct task;
	if (!renku_linux_task_register(&task)) Fatal();
	complete(&f->ready); wait_for_completion(&f->gate);
	struct folio *anchor = shmem_read_folio_gfp(f->file->f_mapping, 0, GFP_KERNEL);
	if (IS_ERR(anchor)) Fatal();
	f->seen[arg->lane] = anchor;
	complete(&f->pinned); wait_for_completion(&f->release);
	for (unsigned i = 0; i < 256; i++) {
		struct page *page = shmem_read_mapping_page(f->file->f_mapping, 0);
		if (IS_ERR(page)) Fatal();
		unsigned *word = (unsigned *)page_address(page) + arg->lane;
		if (page != &anchor->page || *word != (i ? 0x915000U + i - 1 : 0))
			__atomic_fetch_add(&f->errors, 1, __ATOMIC_RELAXED);
		*word = 0x915000U + i;
		put_page(page);
	}
	folio_put(anchor); fput(f->file);
	renku_linux_task_unregister(&task); complete(&f->done);
}
#ifdef RENKU_NATIVE
static int32 ShmemWorkerEntry(void *cookie) { ShmemWorkerBody(cookie); return B_OK; }
static worker_id SpawnShmemWorker(struct shmem_arg *arg)
{
	thread_id id = spawn_kernel_thread(ShmemWorkerEntry, "i915 shmem file reader", B_NORMAL_PRIORITY, arg);
	if (id < B_OK || resume_thread(id) != B_OK) Fatal();
	return id;
}
#else
static void *ShmemWorkerEntry(void *cookie) { ShmemWorkerBody(cookie); return NULL; }
static worker_id SpawnShmemWorker(struct shmem_arg *arg)
{
	pthread_t id; if (pthread_create(&id, NULL, ShmemWorkerEntry, arg)) Fatal(); return id;
}
#endif
static void ShmemCases(struct completion_report *r)
{
	CHECK(r, PTR_ERR(shmem_file_setup(NULL, PAGE_SIZE, 0)) == -EINVAL);
	CHECK(r, PTR_ERR(shmem_file_setup("bad size", -1, 0)) == -EINVAL);
	CHECK(r, PTR_ERR(shmem_file_setup("oversized", LLONG_MAX, 0)) == -EFBIG);
	CHECK(r, PTR_ERR(shmem_file_setup("unknown flags", PAGE_SIZE, 1)) == -EINVAL);
	struct file *empty = shmem_file_setup("empty", 0, 0);
	CHECK(r, !IS_ERR(empty)); if (IS_ERR(empty)) Fatal();
	r->shmem_allocations++;
	CHECK(r, i_size_read(file_inode(empty)) == 0);
	CHECK(r, PTR_ERR(shmem_read_mapping_page(empty->f_mapping, 0)) == -EINVAL);
	fput(empty); CHECK(r, renku_linux_pages_drain(5000) == 0); r->shmem_frees++;

	struct file *file = shmem_file_setup("i915", PAGE_SIZE + 19, VM_NORESERVE);
	CHECK(r, !IS_ERR(file)); if (IS_ERR(file)) Fatal();
	r->shmem_allocations++;
	struct address_space *mapping = file->f_mapping;
	CHECK(r, file_count(file) == 1 && mapping->host == file_inode(file)
		&& mapping->host->i_mapping == mapping && i_size_read(mapping->host) == PAGE_SIZE + 19);
	CHECK(r, get_file(file) == file && file_count(file) == 2);
	fput(file); CHECK(r, file_count(file) == 1);
	CHECK(r, mapping_gfp_mask(mapping) == GFP_KERNEL);
	mapping_set_gfp_mask(mapping, GFP_KERNEL | __GFP_NOWARN);
	CHECK(r, mapping_gfp_constraint(mapping, ~__GFP_RECLAIM) == (__GFP_IO | __GFP_FS | __GFP_NOWARN));
	CHECK(r, PTR_ERR(shmem_read_mapping_page(mapping, 0)) == -EOPNOTSUPP);
	mapping_set_gfp_mask(mapping, GFP_KERNEL);
	CHECK(r, PTR_ERR(shmem_read_folio_gfp(mapping, 0, GFP_ATOMIC)) == -EOPNOTSUPP);
	CHECK(r, PTR_ERR(shmem_read_folio_gfp(mapping, 2, GFP_KERNEL)) == -EINVAL);
	CHECK(r, PTR_ERR(shmem_read_folio_gfp(mapping, ULONG_MAX, GFP_KERNEL)) == -EINVAL);
	preempt_disable();
	CHECK(r, PTR_ERR(shmem_read_folio_gfp(mapping, 0, GFP_KERNEL)) == -EOPNOTSUPP);
	CHECK(r, PTR_ERR(shmem_file_setup("atomic", PAGE_SIZE, 0)) == -EOPNOTSUPP);
	preempt_enable();
	struct folio *folio = shmem_read_folio_gfp(mapping, 1, GFP_KERNEL);
	CHECK(r, !IS_ERR(folio)); if (IS_ERR(folio)) Fatal();
#ifdef RENKU_NATIVE
	area_id area = area_for(page_address(&folio->page)); area_info info;
	CHECK(r, area >= B_OK && get_area_info(area, &info) == B_OK);
	CHECK(r, info.lock == B_NO_LOCK && info.size == 2 * PAGE_SIZE
		&& (info.protection & B_OVERCOMMITTING_AREA) != 0);
	CHECK(r, (char *)page_address(&folio->page) == (char *)info.address + PAGE_SIZE);
#endif
	unsigned char *data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE; i++) {
		CHECK(r, data[i] == 0); data[i] = (unsigned char)(i ^ 0x5a);
	}
	kunmap_local(data); folio_put(folio);
	CHECK(r, renku_linux_pages_drain(5000) == 0);
	folio = shmem_read_folio_gfp(mapping, 1, GFP_KERNEL);
	CHECK(r, !IS_ERR(folio)); if (IS_ERR(folio)) Fatal();
	data = kmap_local_folio(folio, 0);
	for (unsigned i = 0; i < PAGE_SIZE; i++) CHECK(r, data[i] == (unsigned char)(i ^ 0x5a));
	kunmap_local(data);

	struct shmem_fixture f = {.file = file};
	init_completion(&f.ready); init_completion(&f.gate); init_completion(&f.pinned);
	init_completion(&f.release); init_completion(&f.done);
	struct shmem_arg args[2] = {{&f, 0}, {&f, 1}};
	get_file(file); get_file(file);
	worker_id ids[2] = {SpawnShmemWorker(&args[0]), SpawnShmemWorker(&args[1])};
	CHECK(r, wait_for_completion_timeout(&f.ready, 2000) && wait_for_completion_timeout(&f.ready, 2000));
	complete(&f.gate); complete(&f.gate);
	CHECK(r, wait_for_completion_timeout(&f.pinned, 2000) && wait_for_completion_timeout(&f.pinned, 2000));
	CHECK(r, f.seen[0] == f.seen[1] && folio_ref_count(f.seen[0]) == 2);
	CHECK(r, file_count(file) == 3);
	complete(&f.release); complete(&f.release);
	CHECK(r, wait_for_completion_timeout(&f.done, 2000) && wait_for_completion_timeout(&f.done, 2000));
	Join(ids[0]); Join(ids[1]); CHECK(r, f.errors == 0 && file_count(file) == 1);
	r->shmem_workers = 2; r->shmem_iterations = 512;
	FinishFixture(&f.ready); FinishFixture(&f.gate); FinishFixture(&f.pinned);
	FinishFixture(&f.release); FinishFixture(&f.done);

	fput(file);
	CHECK(r, renku_linux_pages_drain(2) == -ETIMEDOUT);
	CHECK(r, i_size_read(mapping->host) == PAGE_SIZE + 19 && renku_linux_pages_uninit() == -EBUSY);
	data = kmap_local_folio(folio, 0); CHECK(r, data[0] == 0x5a && data[19] == 0x49); kunmap_local(data);
#ifdef RENKU_NATIVE
	CHECK(r, get_area_info(area, &info) == B_OK);
#endif
	folio_put(folio); CHECK(r, renku_linux_pages_drain(5000) == 0); r->shmem_frees++;
#ifdef RENKU_NATIVE
	status_t gone = get_area_info(area, &info);
	CHECK(r, gone == B_BAD_TEAM_ID || gone == B_BAD_VALUE);
#endif
}
#endif
