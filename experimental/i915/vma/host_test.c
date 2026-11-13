/* SPDX-License-Identifier: MIT */
#include "cases.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static bool fail_alloc;
static unsigned live;
void *kmalloc(size_t size, gfp_t flags)
{
	assert(flags == GFP_KERNEL);
	if (fail_alloc) return NULL;
	void *memory = malloc(size);
	if (memory) live++;
	return memory;
}
void kfree(const void *memory) { if (memory) { assert(live); live--; free((void *)memory); } }
void panic(const char *format, ...) { (void)format; abort(); }
bool renku_kpi_warn(const char *format, ...) { (void)format; abort(); }
void renku_linux_rwlock_init(rwlock_t *l) { assert(!pthread_rwlock_init(&l->native, NULL)); }
void renku_linux_read_lock(rwlock_t *l) { assert(!pthread_rwlock_rdlock(&l->native)); }
void renku_linux_read_unlock(rwlock_t *l) { assert(!pthread_rwlock_unlock(&l->native)); }
void renku_linux_write_lock(rwlock_t *l) { assert(!pthread_rwlock_wrlock(&l->native)); }
void renku_linux_write_unlock(rwlock_t *l) { assert(!pthread_rwlock_unlock(&l->native)); }

struct argument { struct rw_stress *stress; bool writer; };
static void *worker(void *cookie)
{ struct argument *a = cookie; rw_stress_run(a->stress, a->writer); return NULL; }

int main(void)
{
	struct vma_report report = {};
	vma_cases(&report);
	assert(report.checks > 500 && !report.failures && !live);
	struct drm_vma_offset_node node;
	unsigned char identities[2];
	struct drm_file *tag = (struct drm_file *)&identities[0];
	struct drm_file *other = (struct drm_file *)&identities[1];
	drm_vma_node_reset(&node);
	fail_alloc = true;
	assert(drm_vma_node_allow(&node, tag) == -ENOMEM && !live);
	assert(!drm_vma_node_is_allowed(&node, tag));
	fail_alloc = false;
	assert(drm_vma_node_allow(&node, tag) == 0 && live == 1);
	fail_alloc = true;
	/* Existing access needs no new record even when preallocation fails. */
	assert(drm_vma_node_allow(&node, tag) == 0 && live == 1);
	assert(drm_vma_node_allow(&node, other) == -ENOMEM && live == 1);
	drm_vma_node_revoke(&node, tag);
	assert(drm_vma_node_is_allowed(&node, tag));
	drm_vma_node_revoke(&node, tag);
	assert(!live);
	struct rw_stress stress = {.inverse = ~0u};
	rwlock_init(&stress.lock);
	struct argument args[] = {{&stress, false}, {&stress, false}, {&stress, true}};
	pthread_t threads[3];
	for (unsigned i = 0; i < 3; i++) assert(!pthread_create(&threads[i], NULL, worker, &args[i]));
	for (unsigned i = 0; i < 3; i++) assert(!pthread_join(threads[i], NULL));
	assert(!stress.errors && stress.value == 4000 && stress.max_readers == 2);
	assert(!pthread_rwlock_destroy(&stress.lock.native));
	printf("PASS whole DRM VMA manager: %u checks, OOM/access controls, 12000 lock iterations, readers=%u\n",
		report.checks, stress.max_readers);
	return 0;
}
