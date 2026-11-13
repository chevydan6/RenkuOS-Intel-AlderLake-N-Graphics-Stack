/* SPDX-License-Identifier: MIT */
/* Serialized native-API model, compiled with real RenkuOS declarations.
 * It tests state transitions, not actual interrupt masking or concurrency. */
#include <linux/preempt.h>
#include <linux/spinlock.h>
#include <linux/irqflags.h>
#include <native_irq_scopes.h>
static int32 cpu;
static bool enabled[2] = {true, true};
static bool area_live;
static unsigned char area_memory[4096] __aligned(4096);
static void Bad(void) { __builtin_trap(); }
void panic(const char *format, ...) { (void)format; Bad(); }
int32 smp_get_current_cpu(void) { return cpu; }
int32 smp_get_num_cpus(void) { return 2; }
bool interrupts_enabled(void) { return enabled[cpu]; }
cpu_status disable_interrupts(void)
{ bool old = enabled[cpu]; enabled[cpu] = false; return old; }
void restore_interrupts(cpu_status saved) { enabled[cpu] = saved != 0; }
area_id create_area(const char *name, void **address, uint32 spec, size_t size,
	uint32 lock, uint32 protection)
{
	(void)name;
	if (area_live || size != sizeof(area_memory) || spec != B_ANY_KERNEL_ADDRESS
		|| lock != B_FULL_LOCK || protection != (B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA)) Bad();
	area_live = true; *address = area_memory; return 7;
}
status_t delete_area(area_id area)
{ if (area != 7 || !area_live || !enabled[cpu]) Bad(); area_live = false; return B_OK; }
void acquire_spinlock(spinlock *lock)
{ if (enabled[cpu] || lock->lock) Bad(); lock->lock = 1; }
void release_spinlock(spinlock *lock)
{ if (enabled[cpu] || lock->lock != 1) Bad(); lock->lock = 0; }
void acquire_read_spinlock(rw_spinlock *lock)
{ if (enabled[cpu] || lock->lock < 0) Bad(); lock->lock++; }
void release_read_spinlock(rw_spinlock *lock)
{ if (enabled[cpu] || lock->lock <= 0) Bad(); lock->lock--; }
void acquire_write_spinlock(rw_spinlock *lock)
{ if (enabled[cpu] || lock->lock) Bad(); lock->lock = -1; }
void release_write_spinlock(rw_spinlock *lock)
{ if (enabled[cpu] || lock->lock != -1) Bad(); lock->lock = 0; }

#define EXPECT(x) do { if (!(x)) return 80; } while (0)
int main(void)
{
	EXPECT(renku_linux_irq_backend_init() == B_OK);
	EXPECT(renku_linux_irq_backend_init() == B_BUSY);
	spinlock_t spin; rwlock_t rw;
	spin_lock_init(&spin); rwlock_init(&rw);
	EXPECT(preemptible() && preempt_count() == 0);
	preempt_disable(); raw_spin_lock_irq(&spin); raw_spin_unlock_irq(&spin);
	/* This distinct code is required by the premature-enable regression control. */
	if (enabled[0]) return 90;
	EXPECT(preempt_count() == 1);
	unsigned long saved;
	local_irq_save(saved); EXPECT(saved == 1); local_irq_restore(saved);
	EXPECT(!enabled[0]);
	preempt_enable(); EXPECT(enabled[0]);
	preempt_disable(); local_irq_disable(); preempt_enable();
	EXPECT(!enabled[0] && preempt_count() == 0); local_irq_enable();
	local_irq_disable(); preempt_disable(); local_irq_enable();
	EXPECT(!enabled[0]); preempt_enable(); EXPECT(enabled[0]);
	spin_lock(&spin); preempt_disable(); spin_unlock(&spin);
	EXPECT(!enabled[0] && preempt_count() == 1); preempt_enable(); EXPECT(enabled[0]);
	preempt_disable(); read_lock(&rw); preempt_enable();
	EXPECT(!enabled[0] && preempt_count() == 1); read_unlock(&rw); EXPECT(enabled[0]);
	local_irq_save(saved);
	unsigned long nested;
	raw_spin_lock_irqsave(&spin, nested); preempt_disable();
	raw_spin_unlock_irqrestore(&spin, nested); preempt_enable();
	EXPECT(!enabled[0]); local_irq_restore(saved); EXPECT(enabled[0]);
	/* Simulate two independent CPUs with overlapping scopes, not two host
	 * threads. Separate native VMA tests cover actual concurrent readers. */
	preempt_disable();
	cpu = 1; EXPECT(preempt_count() == 0 && enabled[1]);
	preempt_disable(); preempt_disable();
	EXPECT(preempt_count() == 2 && !enabled[1]);
	cpu = 0; EXPECT(preempt_count() == 1 && !enabled[0]);
	EXPECT(renku_linux_irq_backend_uninit() == B_BUSY);
	preempt_enable(); EXPECT(enabled[0]);
	cpu = 1; preempt_enable(); EXPECT(!enabled[1]); preempt_enable(); EXPECT(enabled[1]);
	EXPECT(renku_linux_irq_backend_uninit() == B_OK && !area_live);
	return 0;
}
