/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_MUTEX_H
#define RENKU_LINUX_MUTEX_H
#include <kpi_base.h>
#ifndef RENKU_NATIVE
#include <pthread.h>
#endif
struct mutex {
#ifdef RENKU_NATIVE
	sem_id semaphore;
#else
	pthread_mutex_t native;
#endif
};
void renku_linux_mutex_init(struct mutex *);
void renku_linux_mutex_destroy(struct mutex *);
void renku_linux_mutex_lock(struct mutex *);
void renku_linux_mutex_unlock(struct mutex *);
#define mutex_init renku_linux_mutex_init
#define mutex_destroy renku_linux_mutex_destroy
#define mutex_lock renku_linux_mutex_lock
#define mutex_unlock renku_linux_mutex_unlock
#endif
