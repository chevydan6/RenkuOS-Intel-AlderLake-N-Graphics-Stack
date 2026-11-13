/* SPDX-License-Identifier: MIT */
#include <linux/rcupdate.h>
#include <linux/spinlock.h>
#include <linux/preempt.h>
#include <native_irq_scopes.h>
#ifndef RENKU_NATIVE
#error This RCU profile requires actual native IRQ and reader/writer locks
#endif
extern int32 smp_get_current_cpu(void);
extern int32 smp_get_num_cpus(void);
static rwlock_t sReaders;
static unsigned sDepth[CONFIG_NR_CPUS];
static bool sReady;
static void Invalid(const char *reason)
{ panic("i915 RCU: %s", reason); __builtin_trap(); }
static unsigned CPU(void)
{
	int32 cpu = smp_get_current_cpu();
	if (!sReady || cpu < 0 || cpu >= CONFIG_NR_CPUS) Invalid("backend unavailable for CPU");
	return cpu;
}
status_t renku_linux_rcu_backend_init(void)
{
	if (sReady) return B_BUSY;
	int32 count = smp_get_num_cpus();
	if (count <= 0 || count > CONFIG_NR_CPUS) return B_BAD_VALUE;
	memset(sDepth, 0, sizeof(sDepth));
	rwlock_init(&sReaders);
	sReady = true;
	return B_OK;
}
status_t renku_linux_rcu_backend_uninit(void)
{
	/* Owner excludes new and running users before shutdown. The depth check
	 * diagnoses a live read section; it is not an unload reference protocol. */
	if (!sReady) return B_BAD_VALUE;
	for (unsigned cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		if (__atomic_load_n(&sDepth[cpu], __ATOMIC_ACQUIRE)) return B_BUSY;
	sReady = false;
	return B_OK;
}
void rcu_read_lock(void)
{
	/* Writer-preferring native locks can block new readers. Forbid outer
	 * entry under an unrelated atomic/spin/IRQ scope to avoid introducing
	 * an RCU -> other-lock -> RCU lock-order cycle. Nested reads skip the RW
	 * acquisition and remain legal inside their existing read scope. */
	if (!rcu_read_lock_held() && !preemptible())
		Invalid("outer read requires IRQ-enabled task context");
	renku_linux_irq_mask_enter(false);
	unsigned cpu = CPU();
	unsigned depth = __atomic_load_n(&sDepth[cpu], __ATOMIC_RELAXED);
	if (depth == UINT_MAX) Invalid("read nesting overflow");
	/* A nested reader must not reacquire behind a waiting writer: that writer
	 * is waiting for this CPU's outer read section to finish. */
	if (!depth) acquire_read_spinlock(&sReaders.raw_lock.native);
	__atomic_store_n(&sDepth[cpu], depth + 1, __ATOMIC_RELEASE);
}
void rcu_read_unlock(void)
{
	if (!renku_linux_irqs_disabled()) Invalid("read section lost IRQ exclusion");
	unsigned cpu = CPU();
	unsigned depth = __atomic_load_n(&sDepth[cpu], __ATOMIC_RELAXED);
	if (!depth) Invalid("unmatched read unlock");
	__atomic_store_n(&sDepth[cpu], depth - 1, __ATOMIC_RELEASE);
	if (depth == 1) release_read_spinlock(&sReaders.raw_lock.native);
	renku_linux_irq_mask_leave(false);
}
bool rcu_read_lock_held(void)
{
	if (!sReady) return false;
	cpu_status saved = disable_interrupts();
	bool held = __atomic_load_n(&sDepth[CPU()], __ATOMIC_ACQUIRE) != 0;
	restore_interrupts(saved);
	return held;
}
void synchronize_rcu(void)
{
	if (!sReady || !preemptible() || rcu_read_lock_held())
		Invalid("grace period requires sleepable task context outside readers");
	/* Native writer exclusion waits for every preexisting read scope. It is
	 * stronger than Linux RCU: new readers can also wait behind this writer.
	 * Both native IRQ-off scopes and this wait require short read sections. */
	write_lock(&sReaders);
	write_unlock(&sReaders);
}
void renku_linux_rcu_require(bool protected_by_lock)
{
	if (!protected_by_lock && !rcu_read_lock_held()) Invalid("unprotected dereference");
}
