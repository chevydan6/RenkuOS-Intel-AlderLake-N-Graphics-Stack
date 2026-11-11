/* Host kernel shims; the implementation under test is the unmodified patched C. */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <KernelExport.h>
#include DPC_SOURCE
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
static pthread_t worker;
static int tokens, deleted, run, entered, unblock, completed;
static int wakeFail, startFail, spawnFail, semFail, joinFail, badExit, acquireFail;
static int deleteCalls, joinCalls, frees;
static atomic_int callbacks;
static int32 (*entry)(void*); static void *entryArg;
static _Thread_local int tid = 1;
static dpc_queue *q;
static int closeOnWake, nestedDelete, selfDelete, requeue, blockCallback;
static status_t closeStatus, selfStatus;
/* Track production frees without destroying retained backing in the harness. */
cpu_status disable_interrupts(void) { return 0; }
void restore_interrupts(cpu_status s) { (void)s; }
void acquire_spinlock(spinlock *s) { while (__atomic_exchange_n(s,1,__ATOMIC_ACQUIRE)) sched_yield(); }
void release_spinlock(spinlock *s) { __atomic_store_n(s,0,__ATOMIC_RELEASE); }
sem_id create_sem(int n,const char *name) { (void)name; tokens=n; return 7; }
status_t acquire_sem(sem_id id) {
 assert(id==7); pthread_mutex_lock(&m);
 while (!tokens && !deleted && !acquireFail) pthread_cond_wait(&cv,&m);
 int result = deleted || acquireFail ? B_ERROR : B_OK;
 if (result==B_OK) tokens--;
 pthread_mutex_unlock(&m); return result;
}
status_t release_sem_etc(sem_id id,int n,int flags) {
 assert(id==7); (void)flags;
 if (closeOnWake) { closeOnWake=0; closeStatus=delete_dpc_queue(q); }
 if (wakeFail) return B_ERROR;
 pthread_mutex_lock(&m); tokens+=n; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m); return B_OK;
}
status_t delete_sem(sem_id id) {
 assert(id==7); deleteCalls++;
 if (nestedDelete) { nestedDelete=0; assert(delete_dpc_queue(q)==B_BUSY); }
 if (semFail) return B_ERROR;
 pthread_mutex_lock(&m); deleted=1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m); return B_OK;
}
static void *workerMain(void *arg) {
 (void)arg; tid=42; pthread_mutex_lock(&m);
 while (!run) pthread_cond_wait(&cv,&m);
 pthread_mutex_unlock(&m);
 return (void*)(long)entry(entryArg);
}
thread_id spawn_kernel_thread(int32(*fn)(void*),const char *name,int32 priority,void *arg) {
 (void)name; (void)priority; entry=fn; entryArg=arg; return spawnFail ? B_ERROR : 42;
}
status_t resume_thread(thread_id id) {
 assert(id==42); if (startFail) return B_ERROR;
 assert(pthread_create(&worker,NULL,workerMain,NULL)==0); return B_OK;
}
status_t wait_for_thread(thread_id id,status_t *out) {
 assert(id==42); joinCalls++; if (joinFail) return B_ERROR;
 void *result; assert(pthread_join(worker,&result)==0);
 *out=badExit ? B_ERROR : (int)(long)result; return B_OK;
}
thread_id find_thread(const char *name) { (void)name; return tid; }
static void allowWorker(void) { pthread_mutex_lock(&m); run=1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m); }
static void cb(void *arg) {
 assert(arg==&callbacks); assert(q->executing==1);
 /* Reentrant APIs prove the callback is outside the queue spinlock. */
 if (selfDelete) { selfStatus=delete_dpc_queue(q); assert(!q->closing && !q->deleting); }
 if (requeue && atomic_load(&callbacks)==0) assert(queue_dpc(q,cb,arg)==B_OK);
 atomic_fetch_add(&callbacks,1);
 pthread_mutex_lock(&m); entered=1; pthread_cond_broadcast(&cv);
 while (blockCallback && !unblock) pthread_cond_wait(&cv,&m);
 completed++; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
}
static void create(void) { void *h=NULL; assert(new_dpc_queue(&h,"test",1)==B_OK); q=h; }
static void retain(void) { assert(q && q->uncertain); assert(queue_dpc(q,cb,&callbacks)==B_CANCELED); int d=deleteCalls,j=joinCalls; assert(delete_dpc_queue(q)!=B_OK); assert(d==deleteCalls && j==joinCalls); }
static void *closer(void *unused) { (void)unused; closeStatus=delete_dpc_queue(q); return NULL; }
int main(int argc,char **argv) {
 assert(argc==2); const char *s=argv[1];
 if (!strcmp(s,"start") || !strcmp(s,"spawn-cleanup")) {
  startFail=!strcmp(s,"start"); spawnFail=!startFail; semFail=spawnFail;
  void *h=NULL; assert(new_dpc_queue(&h,"test",1)!=B_OK && h); q=h; retain(); assert(joinCalls==0); puts(s); return 0;
 }
 create();
 if (!strcmp(s,"full")) {
  assert(queue_dpc(NULL,cb,&callbacks)==B_BAD_VALUE); assert(queue_dpc(q,NULL,NULL)==B_BAD_VALUE);
  for(int i=0;i<64;i++) assert(queue_dpc(q,cb,&callbacks)==B_OK);
  assert(queue_dpc(q,cb,&callbacks)==B_NO_MEMORY && q->count==64); allowWorker();
  assert(delete_dpc_queue(q)==B_OK); assert(callbacks==64);
 } else if (!strcmp(s,"wake") || !strcmp(s,"wake-close")) {
  wakeFail=!strcmp(s,"wake"); closeOnWake=!wakeFail;
  assert(queue_dpc(q,cb,&callbacks)==B_OK); assert(q->count==1 && q->wakers==0);
  if(closeOnWake==0 && !wakeFail) assert(closeStatus==B_BUSY && deleteCalls==0);
  retain();
 } else if (!strcmp(s,"sem") || !strcmp(s,"join") || !strcmp(s,"exit")) {
  semFail=!strcmp(s,"sem"); joinFail=!strcmp(s,"join"); badExit=!strcmp(s,"exit");
  assert(queue_dpc(q,cb,&callbacks)==B_OK); allowWorker(); assert(delete_dpc_queue(q)!=B_OK); retain();
 } else if (!strcmp(s,"executing")) {
  blockCallback=1; assert(queue_dpc(q,cb,&callbacks)==B_OK); allowWorker();
  pthread_mutex_lock(&m); while(!entered) pthread_cond_wait(&cv,&m); pthread_mutex_unlock(&m);
  pthread_t c; assert(pthread_create(&c,NULL,closer,NULL)==0);
  pthread_mutex_lock(&m); while(!deleted) pthread_cond_wait(&cv,&m);
  assert(completed==0); assert(delete_dpc_queue(q)==B_BUSY);
  assert(queue_dpc(q,cb,&callbacks)==B_CANCELED); unblock=1; pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
  pthread_join(c,NULL); assert(closeStatus==B_OK && callbacks==1);
 } else if (!strcmp(s,"requeue") || !strcmp(s,"self")) {
  requeue=!strcmp(s,"requeue"); selfDelete=!requeue;
  assert(queue_dpc(q,cb,&callbacks)==B_OK); allowWorker();
  pthread_mutex_lock(&m); while(completed<(requeue?2:1)) pthread_cond_wait(&cv,&m); pthread_mutex_unlock(&m);
  if(selfDelete) assert(selfStatus==B_NOT_ALLOWED);
  assert(delete_dpc_queue(q)==B_OK); assert(callbacks==(requeue?2:1));
 } else if (!strcmp(s,"acquire")) {
  acquireFail=1; allowWorker(); pthread_mutex_lock(&m); pthread_cond_broadcast(&cv); pthread_mutex_unlock(&m);
  void *result; pthread_join(worker,&result); assert((long)result==B_OK); retain(); assert(joinCalls==0);
 } else {
  assert(!strcmp(s,"drain")); nestedDelete=1;
  for(int i=0;i<4;i++) assert(queue_dpc(q,cb,&callbacks)==B_OK);
  allowWorker(); assert(delete_dpc_queue(q)==B_OK); assert(callbacks==4 && joinCalls==1 && deleteCalls==1);
 }
 (void)frees; puts(s); return 0;
}
