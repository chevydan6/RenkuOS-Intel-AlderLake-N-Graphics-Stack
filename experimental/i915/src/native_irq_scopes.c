/* SPDX-License-Identifier: MIT */
#include <native_irq_scopes.h>
#ifndef RENKU_NATIVE
#error This adapter requires the native kernel
#endif
extern int32 smp_get_current_cpu(void);
extern int32 smp_get_num_cpus(void);
extern bool interrupts_enabled(void);

/* Physical IRQ masking implements short native non-preemptible sections.
 * Keep the requested Linux IRQ restore state separate: an inner _irq unlock
 * must not physically enable IRQs while an outer preempt/read/spin scope lives. */
struct irq_scope { unsigned masks, preempt; cpu_status requested; };
static struct irq_scope *sScopes;
static int32 sCPUCount;
static area_id sArea = -1;
static void Invalid(const char *reason)
{ panic("i915 IRQ scope: %s", reason); __builtin_trap(); }

/* Driver lifecycle excludes new users before shutdown. */
status_t renku_linux_irq_backend_init(void)
{
	if (sArea >= B_OK) return B_BUSY;
	int32 count = smp_get_num_cpus();
	if (count <= 0 || (size_t)count > (SIZE_MAX - B_PAGE_SIZE) / sizeof(*sScopes)) return B_BAD_VALUE;
	size_t size = ((size_t)count * sizeof(*sScopes) + B_PAGE_SIZE - 1) & ~(size_t)(B_PAGE_SIZE - 1);
	void *memory = NULL;
	area_id area = create_area("i915 native IRQ scopes", &memory, B_ANY_KERNEL_ADDRESS,
		size, B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) return area;
	memset(memory, 0, size);
	sScopes = memory; sCPUCount = count; sArea = area;
	return B_OK;
}
status_t renku_linux_irq_backend_uninit(void)
{
	if (sArea < B_OK) return B_BAD_VALUE;
	for (int32 cpu = 0; cpu < sCPUCount; cpu++)
		if (sScopes[cpu].masks || sScopes[cpu].preempt) return B_BUSY;
	status_t status = delete_area(sArea);
	if (status == B_OK) { sArea = -1; sScopes = NULL; sCPUCount = 0; }
	return status;
}
static struct irq_scope *Scope(void)
{
	int32 cpu = smp_get_current_cpu();
	if (!sScopes || cpu < 0 || cpu >= sCPUCount) Invalid("backend not initialized for this CPU");
	return &sScopes[cpu];
}
void renku_linux_irq_mask_enter(bool preempt)
{
	cpu_status saved = disable_interrupts();
	struct irq_scope *scope = Scope();
	if (scope->masks == UINT_MAX) Invalid("nesting overflow");
	if (scope->masks++ == 0) scope->requested = saved;
	if (preempt) scope->preempt++;
	__atomic_signal_fence(__ATOMIC_SEQ_CST);
}
void renku_linux_irq_mask_leave(bool preempt)
{
	if (interrupts_enabled()) Invalid("physical IRQs enabled inside native scope");
	struct irq_scope *scope = Scope();
	if (!scope->masks || (preempt ? !scope->preempt : scope->masks == scope->preempt))
		Invalid("unmatched scope exit");
	if (preempt) scope->preempt--;
	__atomic_signal_fence(__ATOMIC_SEQ_CST);
	if (--scope->masks == 0) restore_interrupts(scope->requested);
}
unsigned int renku_linux_irq_mask_count(void)
{
	cpu_status saved = disable_interrupts();
	unsigned int depth = Scope()->masks;
	restore_interrupts(saved);
	return depth;
}
unsigned long renku_linux_local_irq_save(void)
{
	cpu_status saved = disable_interrupts();
	struct irq_scope *scope = Scope();
	if (scope->masks) { saved = scope->requested; scope->requested = 0; }
	return saved;
}
void renku_linux_local_irq_restore(unsigned long saved)
{
	disable_interrupts();
	struct irq_scope *scope = Scope();
	if (scope->masks) scope->requested = saved != 0;
	else restore_interrupts(saved);
}
bool renku_linux_irqs_disabled(void) { return !interrupts_enabled(); }
