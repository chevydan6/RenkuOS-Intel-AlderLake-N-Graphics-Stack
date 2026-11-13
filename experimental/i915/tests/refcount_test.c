/* SPDX-License-Identifier: MIT */
#include <linux/kref.h>
#include <linux/mutex.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned int warnings, releases;
bool renku_kpi_warn(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	__atomic_fetch_add(&warnings, 1, __ATOMIC_RELAXED);
	return true;
}
static void release(struct kref *ref)
{
	assert(kref_read(ref) == 0);
	__atomic_fetch_add(&releases, 1, __ATOMIC_RELAXED);
}
static void *get_put(void *arg)
{
	struct kref *ref = arg;
	for (unsigned int i = 0; i < 50000; ++i) {
		assert(kref_get_unless_zero(ref));
		assert(kref_put(ref, release) == 0);
	}
	return NULL;
}

/* Last release must acquire writes published by earlier release decrements. */
struct publication {
	struct kref ref;
	unsigned int payload[4];
	unsigned int completions;
};
static void publication_release(struct kref *ref)
{
	struct publication *p = container_of(ref, struct publication, ref);
	for (unsigned int i = 0; i < 4; ++i) assert(p->payload[i] == i + 100);
	__atomic_fetch_add(&p->completions, 1, __ATOMIC_RELAXED);
}
struct writer { struct publication *p; unsigned int index; };
static void *publish(void *arg)
{
	struct writer *w = arg;
	w->p->payload[w->index] = w->index + 100;
	kref_put(&w->p->ref, publication_release);
	return NULL;
}

int main(void)
{
	struct kref ref;
	kref_init(&ref);
	kref_get(&ref);
	assert(kref_read(&ref) == 2);
	assert(kref_put(&ref, release) == 0);
	pthread_t threads[8];
	for (unsigned int i = 0; i < 8; ++i) assert(pthread_create(threads + i, NULL, get_put, &ref) == 0);
	for (unsigned int i = 0; i < 8; ++i) assert(pthread_join(threads[i], NULL) == 0);
	assert(kref_read(&ref) == 1 && releases == 0);
	assert(kref_put(&ref, release) == 1 && releases == 1);
	assert(!kref_get_unless_zero(&ref));

	refcount_t r = REFCOUNT_INIT(1);
	assert(!refcount_dec_not_one(&r));
	assert(refcount_dec_if_one(&r));
	assert(!refcount_dec_if_one(&r));
	refcount_set(&r, 3);
	assert(refcount_dec_not_one(&r) && refcount_read(&r) == 2);
	assert(refcount_sub_and_test(2, &r));

	struct mutex m;
	mutex_init(&m);
	refcount_set(&r, 2);
	assert(!refcount_dec_and_mutex_lock(&r, &m));
	assert(refcount_dec_and_mutex_lock(&r, &m));
	assert(pthread_mutex_trylock(&m.native) != 0); /* function returned with lock held */
	mutex_unlock(&m);
	mutex_destroy(&m);
	spinlock_t s;
	spin_lock_init(&s);
	refcount_set(&r, 2);
	assert(!refcount_dec_and_lock(&r, &s));
	assert(refcount_dec_and_lock(&r, &s));
	assert(pthread_mutex_trylock(&s.rlock.raw_lock.native) != 0);
	spin_unlock(&s);
	assert(pthread_mutex_destroy(&s.rlock.raw_lock.native) == 0);

	for (unsigned int round = 0; round < 100; ++round) {
		struct publication p = { .ref = KREF_INIT(4) };
		struct writer writers[4];
		for (unsigned int i = 0; i < 4; ++i) {
			writers[i] = (struct writer){ &p, i };
			assert(pthread_create(threads + i, NULL, publish, writers + i) == 0);
		}
		for (unsigned int i = 0; i < 4; ++i) assert(pthread_join(threads[i], NULL) == 0);
		assert(p.completions == 1);
	}
	assert(warnings == 0);

	/* Trigger each upstream saturation category, including signed overflow. */
	refcount_set(&r, INT_MAX);
	refcount_inc(&r);
	assert(refcount_read(&r) == (unsigned int)REFCOUNT_SATURATED);
	refcount_set(&r, INT_MAX);
	assert(refcount_inc_not_zero(&r));
	assert(refcount_read(&r) == (unsigned int)REFCOUNT_SATURATED);
	refcount_set(&r, 0);
	refcount_inc(&r);
	assert(refcount_read(&r) == (unsigned int)REFCOUNT_SATURATED);
	refcount_set(&r, 0);
	assert(!refcount_dec_and_test(&r));
	assert(refcount_read(&r) == (unsigned int)REFCOUNT_SATURATED);
	refcount_set(&r, 1);
	refcount_dec(&r);
	assert(refcount_read(&r) == (unsigned int)REFCOUNT_SATURATED);
	refcount_set(&ref.refcount, REFCOUNT_SATURATED);
	assert(!kref_put(&ref, release));
	assert(releases == 1 && warnings == 5);
	printf("CPU refcount: 400000 concurrent get/put pairs, 100 publication rounds, 5 saturation categories; pass\n");
	return 0;
}
