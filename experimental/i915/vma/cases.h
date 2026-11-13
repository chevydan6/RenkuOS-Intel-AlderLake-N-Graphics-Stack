/* SPDX-License-Identifier: MIT */
#ifndef RENKU_VMA_CASES_H
#define RENKU_VMA_CASES_H
#include <drm/drm_vma_manager.h>
#include <linux/log2.h>
#include <linux/slab.h>
#include "test.h"

#define VMA_CHECK(r, c) do { (r)->checks++; if (!(c)) { (r)->failures++; \
	if (!(r)->first_line) (r)->first_line = __LINE__; } } while (0)

static void vma_cases(struct vma_report *r)
{
	VMA_CHECK(r, fls(0) == 0 && fls64(0) == 0 && fls_long(0) == 0);
	for (unsigned i = 0; i < 64; i++) {
		volatile unsigned long value = 1UL << i;
		VMA_CHECK(r, fls64(value) == (int)i + 1);
		VMA_CHECK(r, fls_long(value) == (int)i + 1);
		VMA_CHECK(r, ilog2(value) == (int)i);
		VMA_CHECK(r, is_power_of_2(value));
		VMA_CHECK(r, rounddown_pow_of_two(value) == value);
		if (i < 32) VMA_CHECK(r, fls((int)value) == (int)i + 1);
		if (i > 1 && i < 63) {
			VMA_CHECK(r, !is_power_of_2(value + 1));
			VMA_CHECK(r, roundup_pow_of_two(value + 1) == value * 2);
			VMA_CHECK(r, order_base_2(value + 1) == (int)i + 1);
		}
	}
	VMA_CHECK(r, !is_power_of_2(0) && bits_per(0) == 1 && order_base_2(0) == 0);
	struct drm_vma_offset_manager mgr;
	struct drm_vma_offset_node nodes[3];
	/* Only opaque file identities are needed; do not invent a drm_file layout. */
	unsigned char tags[3];
	struct drm_file *a = (struct drm_file *)&tags[0];
	struct drm_file *b = (struct drm_file *)&tags[1];
	struct drm_file *c = (struct drm_file *)&tags[2];
	drm_vma_offset_manager_init(&mgr, 0x100000, 32);
	for (unsigned i = 0; i < 3; i++) drm_vma_node_reset(&nodes[i]);
	VMA_CHECK(r, drm_vma_offset_add(&mgr, &nodes[0], 8) == 0);
	VMA_CHECK(r, drm_vma_offset_add(&mgr, &nodes[1], 16) == 0);
	VMA_CHECK(r, drm_vma_offset_add(&mgr, &nodes[2], 16) == -ENOSPC);
	VMA_CHECK(r, drm_vma_node_size(&nodes[0]) == 8);
	VMA_CHECK(r, drm_vma_node_offset_addr(&nodes[0]) == (u64)drm_vma_node_start(&nodes[0]) * PAGE_SIZE);
	drm_vma_offset_lock_lookup(&mgr);
	unsigned long start = drm_vma_node_start(&nodes[0]);
	VMA_CHECK(r, drm_vma_offset_lookup_locked(&mgr, start + 1, 7) == &nodes[0]);
	VMA_CHECK(r, drm_vma_offset_lookup_locked(&mgr, start + 1, 8) == NULL);
	VMA_CHECK(r, drm_vma_offset_exact_lookup_locked(&mgr, start, 8) == &nodes[0]);
	VMA_CHECK(r, drm_vma_offset_exact_lookup_locked(&mgr, start + 1, 1) == NULL);
	drm_vma_offset_unlock_lookup(&mgr);
	VMA_CHECK(r, !drm_vma_node_is_allowed(&nodes[0], a));
	VMA_CHECK(r, drm_vma_node_verify_access(&nodes[0], a) == -EACCES);
	VMA_CHECK(r, drm_vma_node_allow(&nodes[0], a) == 0);
	VMA_CHECK(r, drm_vma_node_allow(&nodes[0], a) == 0);
	VMA_CHECK(r, drm_vma_node_allow_once(&nodes[0], b) == 0);
	VMA_CHECK(r, drm_vma_node_allow_once(&nodes[0], b) == 0);
	VMA_CHECK(r, !drm_vma_node_is_allowed(&nodes[0], c));
	VMA_CHECK(r, drm_vma_node_verify_access(&nodes[0], a) == 0);
	drm_vma_node_revoke(&nodes[0], a);
	VMA_CHECK(r, drm_vma_node_is_allowed(&nodes[0], a));
	drm_vma_node_revoke(&nodes[0], a);
	VMA_CHECK(r, !drm_vma_node_is_allowed(&nodes[0], a));
	drm_vma_node_revoke(&nodes[0], b);
	VMA_CHECK(r, !drm_vma_node_is_allowed(&nodes[0], b));
	drm_vma_node_revoke(&nodes[0], b); /* Missing revocation is harmless upstream. */
	for (unsigned i = 0; i < 2; i++) drm_vma_offset_remove(&mgr, &nodes[i]);
	VMA_CHECK(r, drm_vma_node_size(&nodes[0]) == 0);
	VMA_CHECK(r, drm_vma_offset_add(&mgr, &nodes[2], 32) == 0);
	drm_vma_offset_remove(&mgr, &nodes[2]);
	VMA_CHECK(r, drm_mm_clean(&mgr.vm_addr_space_mm));
	drm_vma_offset_manager_destroy(&mgr);
#ifndef RENKU_NATIVE
	VMA_CHECK(r, pthread_rwlock_destroy(&mgr.vm_lock.raw_lock.native) == 0);
	for (unsigned i = 0; i < 3; i++)
		VMA_CHECK(r, pthread_rwlock_destroy(&nodes[i].vm_lock.raw_lock.native) == 0);
#endif
}

struct rw_stress {
	rwlock_t lock;
	unsigned value, inverse, readers, max_readers, errors;
};
static void rw_stress_run(struct rw_stress *s, bool writer)
{
#ifdef RENKU_NATIVE
	extern bool interrupts_enabled(void);
	bool initial_irq = interrupts_enabled();
#endif
	for (unsigned i = 0; i < 4000; i++) {
		if (writer) {
			write_lock(&s->lock);
#ifdef RENKU_NATIVE
			if (interrupts_enabled()) __atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
#endif
			if (__atomic_load_n(&s->readers, __ATOMIC_RELAXED))
				__atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
			s->value++;
			for (unsigned pause = 0; pause < 64; pause++) __asm__ volatile("pause" ::: "memory");
			s->inverse = ~s->value;
			write_unlock(&s->lock);
		} else {
			read_lock(&s->lock);
#ifdef RENKU_NATIVE
			if (interrupts_enabled()) __atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
#endif
			unsigned readers = __atomic_add_fetch(&s->readers, 1, __ATOMIC_RELAXED);
			unsigned maximum = __atomic_load_n(&s->max_readers, __ATOMIC_RELAXED);
			while (maximum < readers && !__atomic_compare_exchange_n(&s->max_readers,
				&maximum, readers, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {}
			if (s->inverse != ~s->value) __atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
			for (unsigned pause = 0; pause < 64; pause++) __asm__ volatile("pause" ::: "memory");
			__atomic_fetch_sub(&s->readers, 1, __ATOMIC_RELAXED);
			read_unlock(&s->lock);
		}
#ifdef RENKU_NATIVE
		if (interrupts_enabled() != initial_irq) __atomic_fetch_add(&s->errors, 1, __ATOMIC_RELAXED);
#endif
	}
}
#endif
