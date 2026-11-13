/* SPDX-License-Identifier: MIT */
#include <linux/spinlock.h>
#ifndef RENKU_NATIVE
#error This adapter requires the native kernel
#endif

/* Native declarations from private/kernel/smp.h. That header also contains C++
 * CPUSet declarations, so the C adapter repeats these two exported prototypes. */
extern int32 smp_get_num_cpus(void);
extern int32 smp_get_current_cpu(void);

struct irq_scope {
	unsigned depth;
	cpu_status saved;
};
static struct irq_scope *sScopes;
static int32 sCPUCount;
static area_id sArea = -1;

/* Module lifecycle serializes startup/shutdown and excludes new lock users. */
status_t renku_linux_rw_backend_init(void)
{
	if (sArea >= B_OK) return B_BUSY;
	int32 count = smp_get_num_cpus();
	if (count <= 0 || (size_t)count > (SIZE_MAX - B_PAGE_SIZE) / sizeof(*sScopes))
		return B_BAD_VALUE;
	size_t size = ((size_t)count * sizeof(*sScopes) + B_PAGE_SIZE - 1)
		& ~(size_t)(B_PAGE_SIZE - 1);
	void *memory = NULL;
	area_id area = create_area("i915 rwlock IRQ scopes", &memory,
		B_ANY_KERNEL_ADDRESS, size, B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) return area;
	memset(memory, 0, size);
	sCPUCount = count;
	sScopes = memory;
	sArea = area;
	return B_OK;
}

status_t renku_linux_rw_backend_uninit(void)
{
	if (sArea < B_OK) return B_BAD_VALUE;
	for (int32 cpu = 0; cpu < sCPUCount; cpu++)
		if (sScopes[cpu].depth != 0) return B_BUSY;
	status_t status = delete_area(sArea);
	if (status == B_OK) {
		sArea = -1;
		sScopes = NULL;
		sCPUCount = 0;
	}
	return status;
}

static struct irq_scope *CurrentScope(void)
{
	int32 cpu = smp_get_current_cpu();
	if (sScopes == NULL || cpu < 0 || cpu >= sCPUCount) {
		panic("i915: rwlock backend unavailable for CPU %ld", (long)cpu);
		__builtin_trap();
	}
	return &sScopes[cpu];
}

static void Enter(void)
{
	cpu_status saved = disable_interrupts();
	/* IRQ exclusion pins native execution to this CPU. Readers on different
	 * CPUs need separate saved states; one field in rwlock_t would race. */
	struct irq_scope *scope = CurrentScope();
	if (scope->depth == UINT_MAX) {
		panic("i915: rwlock IRQ nesting overflow");
		__builtin_trap();
	}
	if (scope->depth++ == 0) scope->saved = saved;
}

static void Leave(void)
{
	struct irq_scope *scope = CurrentScope();
	if (scope->depth == 0) {
		panic("i915: unmatched rwlock unlock");
		__builtin_trap();
	}
	if (--scope->depth == 0) restore_interrupts(scope->saved);
}

void renku_linux_rwlock_init(rwlock_t *lock)
{ B_INITIALIZE_RW_SPINLOCK(&lock->native); }
void renku_linux_read_lock(rwlock_t *lock)
{ Enter(); acquire_read_spinlock(&lock->native); }
void renku_linux_read_unlock(rwlock_t *lock)
{ release_read_spinlock(&lock->native); Leave(); }
void renku_linux_write_lock(rwlock_t *lock)
{ Enter(); acquire_write_spinlock(&lock->native); }
void renku_linux_write_unlock(rwlock_t *lock)
{ release_write_spinlock(&lock->native); Leave(); }
