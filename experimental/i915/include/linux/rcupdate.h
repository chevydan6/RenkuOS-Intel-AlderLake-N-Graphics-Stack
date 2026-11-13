/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_RCUPDATE_H
#define RENKU_LINUX_RCUPDATE_H
#include <kpi_base.h>
/* Callback metadata only. No grace period, read-side protection or call_rcu
 * service is supplied; those lifetime interfaces remain unresolved. */
struct rcu_head {
	struct rcu_head *next;
	void (*func)(struct rcu_head *);
};
#define rcu_dereference(p) smp_load_acquire(&(p))
#define rcu_access_pointer(p) READ_ONCE(p)
#define rcu_pointer_handoff(p) (p)
#endif
