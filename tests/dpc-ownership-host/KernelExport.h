#ifndef HOST_KERNEL_EXPORT_H
#define HOST_KERNEL_EXPORT_H
#include <stdbool.h>
#include <stddef.h>
typedef int status_t; typedef int int32; typedef int thread_id; typedef int sem_id; typedef int spinlock; typedef int cpu_status;
#define B_OK 0
#define B_ERROR -1
#define B_BAD_VALUE -2
#define B_NO_MEMORY -3
#define B_CANCELED -4
#define B_BUSY -5
#define B_NOT_ALLOWED -6
#define B_DO_NOT_RESCHEDULE 0
#define B_MODULE_INIT 1
#define B_MODULE_UNINIT 2
#define B_INITIALIZE_SPINLOCK(p) (*(p)=0)
cpu_status disable_interrupts(void); void restore_interrupts(cpu_status); void acquire_spinlock(spinlock*); void release_spinlock(spinlock*);
status_t acquire_sem(sem_id); status_t release_sem_etc(sem_id,int,int); sem_id create_sem(int,const char*); status_t delete_sem(sem_id);
thread_id spawn_kernel_thread(int32(*)(void*),const char*,int32,void*); status_t resume_thread(thread_id); status_t wait_for_thread(thread_id,status_t*); thread_id find_thread(const char*);

#endif
