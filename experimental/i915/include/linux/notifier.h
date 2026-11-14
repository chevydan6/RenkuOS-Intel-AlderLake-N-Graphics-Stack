/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_NOTIFIER_H
#define RENKU_LINUX_NOTIFIER_H
#include <kpi_base.h>
struct notifier_block;
typedef int (*notifier_fn_t)(struct notifier_block *, unsigned long, void *);
struct notifier_block {
	notifier_fn_t notifier_call;
	struct notifier_block *next;
	int priority;
};
/* Adapter-owned chain handles. Notification serialization, reader retirement
 * and callback lifetime must be supplied by the native chain implementation;
 * no Linux SRCU or rw_semaphore layout is assumed here. */
struct renku_notifier_chain;
struct blocking_notifier_head { struct renku_notifier_chain *renku_chain; };
struct atomic_notifier_head { struct renku_notifier_chain *renku_chain; };
struct raw_notifier_head { struct renku_notifier_chain *renku_chain; };
struct srcu_notifier_head { struct renku_notifier_chain *renku_chain; };
int blocking_notifier_chain_register(struct blocking_notifier_head *, struct notifier_block *);
int blocking_notifier_chain_unregister(struct blocking_notifier_head *, struct notifier_block *);
int blocking_notifier_call_chain(struct blocking_notifier_head *, unsigned long, void *);
int atomic_notifier_chain_register(struct atomic_notifier_head *, struct notifier_block *);
int atomic_notifier_chain_unregister(struct atomic_notifier_head *, struct notifier_block *);
int atomic_notifier_call_chain(struct atomic_notifier_head *, unsigned long, void *);
#endif
