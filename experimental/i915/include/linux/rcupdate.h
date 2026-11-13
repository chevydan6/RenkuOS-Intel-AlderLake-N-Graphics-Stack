/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_RCUPDATE_H
#define RENKU_LINUX_RCUPDATE_H
#include <kpi_base.h>
struct rcu_head {
	struct rcu_head *next;
	void (*func)(struct rcu_head *);
};
/* Outer entry requires IRQ-enabled/preemptible task context, outside other
 * spin/atomic scopes. Read sections are short and nonblocking; nested reads are
 * allowed. Native IRQ exclusion is stronger than Linux RCU. Fixed CPU topology, explicit serialized startup/shutdown and excluded
 * users at teardown are required. No NMI or general preemptible-RCU profile. */
#ifdef RENKU_NATIVE
status_t renku_linux_rcu_backend_init(void);
status_t renku_linux_rcu_backend_uninit(void);
#endif
void rcu_read_lock(void);
void rcu_read_unlock(void);
bool rcu_read_lock_held(void);
void synchronize_rcu(void);
void renku_linux_rcu_require(bool);
#define rcu_dereference_check(p, c) ({ renku_linux_rcu_require(!!(c)); smp_load_acquire(&(p)); })
#define rcu_dereference(p) rcu_dereference_check(p, false)
#define rcu_dereference_protected(p, c) ({ BUG_ON(!(c)); READ_ONCE(p); })
#define rcu_access_pointer(p) READ_ONCE(p)
#define rcu_pointer_handoff(p) (p)
/* Callback/encoded-free lifetimes and callback draining are not supplied.
 * Keep these services unresolved until their real implementation exists. */
void renku_linux_call_rcu(struct rcu_head *, void (*)(struct rcu_head *));
void renku_linux_rcu_barrier(void);
#define call_rcu renku_linux_call_rcu
#define rcu_barrier renku_linux_rcu_barrier
#endif
