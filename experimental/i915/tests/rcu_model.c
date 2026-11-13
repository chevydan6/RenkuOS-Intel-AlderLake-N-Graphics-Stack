/* SPDX-License-Identifier: MIT */
/* Actual native RCU/IRQ adapters and target declarations, modeled native APIs.
 * Pthread operations are in a separately compiled host translation unit. */
#include <linux/rcupdate.h>
#include <linux/preempt.h>
#include <linux/spinlock.h>
#include <native_irq_scopes.h>
static _Thread_local int32 cpu;
static _Thread_local bool irq_enabled = true;
static unsigned char area_memory[4096] __aligned(4096);
static bool area_live;
extern void model_init(void), model_finish(void), model_pause(void);
extern void model_read_lock(void *), model_read_unlock(void *);
extern void model_write_lock(void *), model_write_unlock(void *);
extern int model_writer_attempt(void);
extern void model_start(unsigned, void *(*)(void *), void *), model_join(unsigned);
extern int64_t model_now_us(void);
void panic(const char *format, ...) { (void)format; __builtin_trap(); }
int32 smp_get_current_cpu(void) { return cpu; }
int32 smp_get_num_cpus(void) { return 3; }
bool interrupts_enabled(void) { return irq_enabled; }
cpu_status disable_interrupts(void) { bool old = irq_enabled; irq_enabled = false; return old; }
void restore_interrupts(cpu_status saved) { irq_enabled = saved != 0; }
area_id create_area(const char *name, void **address, uint32 spec, size_t size,
	uint32 lock, uint32 protection)
{
	(void)name;
	if (area_live || size != sizeof(area_memory) || spec != B_ANY_KERNEL_ADDRESS
		|| lock != B_FULL_LOCK || protection != (B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA)) __builtin_trap();
	area_live = true; *address = area_memory; return 7;
}
status_t delete_area(area_id id)
{ if (id != 7 || !area_live) __builtin_trap(); area_live = false; return B_OK; }
void acquire_read_spinlock(rw_spinlock *lock)
{ if (irq_enabled) __builtin_trap(); model_read_lock(lock); }
void release_read_spinlock(rw_spinlock *lock)
{ if (irq_enabled) __builtin_trap(); model_read_unlock(lock); }
void acquire_write_spinlock(rw_spinlock *lock)
{ if (irq_enabled) __builtin_trap(); model_write_lock(lock); }
void release_write_spinlock(rw_spinlock *lock)
{ if (irq_enabled) __builtin_trap(); model_write_unlock(lock); }
struct node { unsigned value; };
static struct node *published;
static unsigned ready, nested_request, nested_done, release_reader, writer_done, errors;
static void Until(unsigned *flag)
{
	int64_t end = model_now_us() + 2000000;
	while (!__atomic_load_n(flag, __ATOMIC_ACQUIRE)) {
		if (model_now_us() >= end) __builtin_trap();
		model_pause();
	}
}
static void *Reader(void *unused)
{
	(void)unused; cpu = 1;
	rcu_read_lock();
	struct node *old = rcu_dereference(published);
	__atomic_store_n(&ready, 1, __ATOMIC_RELEASE);
	Until(&nested_request);
	rcu_read_lock();
	if (!rcu_read_lock_held()) __atomic_fetch_add(&errors, 1, __ATOMIC_RELAXED);
	rcu_read_unlock();
	__atomic_store_n(&nested_done, 1, __ATOMIC_RELEASE);
	Until(&release_reader);
	if (old->value != 915) __atomic_fetch_add(&errors, 1, __ATOMIC_RELAXED);
	rcu_read_unlock();
	if (!irq_enabled || rcu_read_lock_held()) __atomic_fetch_add(&errors, 1, __ATOMIC_RELAXED);
	return NULL;
}
static void *Writer(void *unused)
{
	(void)unused; cpu = 2;
	synchronize_rcu();
	__atomic_store_n(&writer_done, 1, __ATOMIC_RELEASE);
	return NULL;
}
#define EXPECT(x) do { if (!(x)) return __LINE__; } while (0)
int main(int argc, char **argv)
{
	
	model_init();
	EXPECT(renku_linux_irq_backend_init() == B_OK);
	EXPECT(renku_linux_rcu_backend_init() == B_OK);
	EXPECT(renku_linux_rcu_backend_init() == B_BUSY);
	EXPECT(!rcu_read_lock_held());
	if (argc > 1) {
		if (argv[1][0] == 'e') { preempt_disable(); rcu_read_lock(); }
		else { rcu_read_lock(); synchronize_rcu(); }
		return 80;
	}
	rcu_read_lock(); rcu_read_lock();
	EXPECT(rcu_read_lock_held() && !irq_enabled && preempt_count() == 2);
	EXPECT(renku_linux_rcu_backend_uninit() == B_BUSY);
	rcu_read_unlock(); EXPECT(rcu_read_lock_held()); rcu_read_unlock();
	EXPECT(irq_enabled && !rcu_read_lock_held() && preempt_count() == 0);
	published = malloc(sizeof(*published)); EXPECT(published != NULL);
	published->value = 915;
	struct node *old = published;
	model_start(0, Reader, NULL); Until(&ready);
	rcu_assign_pointer(published, NULL);
	model_start(1, Writer, NULL);
	int64_t deadline = model_now_us() + 2000000;
	while (!model_writer_attempt() && !__atomic_load_n(&writer_done, __ATOMIC_ACQUIRE)) {
		if (model_now_us() >= deadline) __builtin_trap();
		model_pause();
	}
	bool premature = __atomic_load_n(&writer_done, __ATOMIC_ACQUIRE) != 0;
	__atomic_store_n(&nested_request, 1, __ATOMIC_RELEASE); Until(&nested_done);
	premature |= __atomic_load_n(&writer_done, __ATOMIC_ACQUIRE) != 0;
	__atomic_store_n(&release_reader, 1, __ATOMIC_RELEASE);
	model_join(0); model_join(1);
	EXPECT(writer_done && !errors && !published);
	free(old); /* no reader can retain the object after the grace period */
	EXPECT(renku_linux_rcu_backend_uninit() == B_OK);
	EXPECT(renku_linux_irq_backend_uninit() == B_OK && !area_live);
	model_finish();
	return premature ? 90 : 0;
}
