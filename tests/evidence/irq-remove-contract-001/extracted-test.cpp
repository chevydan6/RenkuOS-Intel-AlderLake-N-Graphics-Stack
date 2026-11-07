
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <mutex>
#include <thread>
using int32 = int32_t;
using status_t = int;
using cpu_status = int;
using bigtime_t = int64_t;
using interrupt_handler = int (*)(void*);
enum interrupt_trigger_mode { B_EDGE_TRIGGERED, B_LEVEL_TRIGGERED };
enum { B_OK = 0, B_BAD_VALUE = -1, B_UNHANDLED_INTERRUPT = 0,
 B_HANDLED_INTERRUPT = 1, B_INVOKE_SCHEDULER = 2, NUM_IO_VECTORS = 1,
 INTERRUPT_TYPE_UNKNOWN, INTERRUPT_TYPE_EXCEPTION, INTERRUPT_TYPE_SYSCALL,
 INTERRUPT_TYPE_IRQ, INTERRUPT_TYPE_ICI, INTERRUPT_TYPE_LOCAL_IRQ };
#define DEBUG_INTERRUPTS 0
#define ASSERT assert
#define ASSERT_PRINT(condition, ...) assert(condition)
#define dprintf(...) ((void)0)
struct io_handler {
 interrupt_handler func;
 void* data;
 io_handler* next;
 bool use_enable_counter;
};
struct Assignment { int32 handlers_count = 0; int cpu = -1; };
struct IRQList { void Remove(Assignment*) {} };
struct cpu_ent {
 std::mutex irqs_lock;
 IRQList irqs;
 bigtime_t interrupt_time = 0, irq_time = 0;
};
struct Vector {
 bool no_lock_vector = false;
 std::mutex vector_lock, load_lock;
 io_handler* handler_list = nullptr;
 int type = INTERRUPT_TYPE_UNKNOWN, enable_count = 0;
 Assignment* assigned_cpu = nullptr;
 bigtime_t last_measure_active = 0;
};
static Vector sVectors[NUM_IO_VECTORS];
static cpu_ent gCPU[1];
static std::atomic<bool> entered{false}, attemptingRemoval{false};
static std::atomic<bool> removed{false}, exited{false};
static thread_local bool isRemover = false;
static bool noLock;

static void acquire_spinlock(std::mutex* lock)
{
 if (isRemover) attemptingRemoval.store(true);
 lock->lock();
}
static void release_spinlock(std::mutex* lock) { lock->unlock(); }
class SpinLocker {
 std::mutex* lock = nullptr;
public:
 SpinLocker() = default;
 explicit SpinLocker(std::mutex& value) { SetTo(value, false); }
 ~SpinLocker() { Unlock(); }
 void Unlock() { if (lock) { lock->unlock(); lock = nullptr; } }
 void SetTo(std::mutex& value, bool) { Unlock(); lock = &value; lock->lock(); }
};
static cpu_status disable_interrupts() { return 0; }
static void restore_interrupts(cpu_status) {}
static bool are_interrupts_enabled() { return false; }
static void arch_int_disable_io_interrupt(int) {}
static int32 atomic_add(int32* value, int32 delta)
{ int32 old = *value; *value += delta; return old; }
static cpu_ent* get_cpu_struct() { return &gCPU[0]; }
static void update_int_load(int) {}
static bigtime_t system_time()
{
 return std::chrono::duration_cast<std::chrono::microseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void wait_for(const std::atomic<bool>& event)
{
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
 while (!event.load()) {
  if (std::chrono::steady_clock::now() >= deadline) std::abort();
  std::this_thread::yield();
 }
}
static int callback(void*)
{
 entered.store(true);
 wait_for(noLock ? removed : attemptingRemoval);
 exited.store(true);
 return B_HANDLED_INTERRUPT;
}
int
io_interrupt_handler(int vector, interrupt_trigger_mode triggerMode)
{
	int status = B_UNHANDLED_INTERRUPT;
	struct io_handler* io;
	bool handled = false;

	bigtime_t start = system_time();

	// exceptions and syscalls have their own handlers
	ASSERT(sVectors[vector].type != INTERRUPT_TYPE_EXCEPTION
		&& sVectors[vector].type != INTERRUPT_TYPE_SYSCALL);

	if (!sVectors[vector].no_lock_vector)
		acquire_spinlock(&sVectors[vector].vector_lock);

#if !DEBUG_INTERRUPTS
	// The list can be empty at this place
	if (sVectors[vector].handler_list == NULL) {
		dprintf("unhandled io interrupt %d\n", vector);
		if (!sVectors[vector].no_lock_vector)
			release_spinlock(&sVectors[vector].vector_lock);
		return B_UNHANDLED_INTERRUPT;
	}
#endif

	// For level-triggered interrupts, we actually handle the return
	// value (ie. B_HANDLED_INTERRUPT) to decide whether or not we
	// want to call another interrupt handler.
	// For edge-triggered interrupts, however, we always need to call
	// all handlers, as multiple interrupts cannot be identified. We
	// still make sure the return code of this function will issue
	// whatever the driver thought would be useful.

	for (io = sVectors[vector].handler_list; io != NULL; io = io->next) {
		status = io->func(io->data);

#if DEBUG_INTERRUPTS
		if (status != B_UNHANDLED_INTERRUPT)
			io->handled_count++;
#endif
		if (triggerMode == B_LEVEL_TRIGGERED && status != B_UNHANDLED_INTERRUPT)
			break;

		if (status == B_HANDLED_INTERRUPT || status == B_INVOKE_SCHEDULER)
			handled = true;
	}

	ASSERT_PRINT(!are_interrupts_enabled(),
		"interrupts enabled after calling handlers for vector %d", vector);

#if DEBUG_INTERRUPTS
	sVectors[vector].trigger_count++;
	if (status != B_UNHANDLED_INTERRUPT || handled) {
		sVectors[vector].handled_count++;
	} else {
		sVectors[vector].unhandled_count++;
		sVectors[vector].ignored_count++;
	}

	if (sVectors[vector].trigger_count > 10000) {
		if (sVectors[vector].ignored_count > 9900) {
			struct io_handler *last = sVectors[vector].handler_list;
			while (last && last->next)
				last = last->next;

			if (last != NULL && last->no_handled_info) {
				// we have an interrupt handler installed that does not
				// know whether or not it has actually handled the interrupt,
				// so this unhandled count is inaccurate and we can't just
				// disable
			} else {
				if (sVectors[vector].handler_list == NULL
					|| sVectors[vector].handler_list->next == NULL) {
					// this interrupt vector is not shared, disable it
					sVectors[vector].enable_count = -100;
					arch_int_disable_io_interrupt(vector);
					dprintf("Disabling unhandled io interrupt %d\n", vector);
				} else {
					// this is a shared interrupt vector, we cannot just disable it
					dprintf("More than 99%% interrupts of vector %d are unhandled\n",
						vector);
				}
			}
		}

		sVectors[vector].trigger_count = 0;
		sVectors[vector].ignored_count = 0;
	}
#endif

	if (!sVectors[vector].no_lock_vector)
		release_spinlock(&sVectors[vector].vector_lock);

	SpinLocker vectorLocker(sVectors[vector].load_lock);
	bigtime_t deltaTime = system_time() - start;
	sVectors[vector].last_measure_active += deltaTime;
	vectorLocker.Unlock();

	cpu_ent* cpu = get_cpu_struct();
	if (sVectors[vector].type == INTERRUPT_TYPE_IRQ
		|| sVectors[vector].type == INTERRUPT_TYPE_ICI
		|| sVectors[vector].type == INTERRUPT_TYPE_LOCAL_IRQ) {
		cpu->interrupt_time += deltaTime;
		if (sVectors[vector].type == INTERRUPT_TYPE_IRQ)
			cpu->irq_time += deltaTime;
	}

	update_int_load(vector);

	if (triggerMode == B_LEVEL_TRIGGERED)
		return status;

	// edge triggered return value

	if (handled)
		return B_HANDLED_INTERRUPT;

	return B_UNHANDLED_INTERRUPT;
}

status_t
remove_io_interrupt_handler(int32 vector, interrupt_handler handler, void *data)
{
	status_t status = B_BAD_VALUE;
	struct io_handler *io = NULL;
	struct io_handler *last = NULL;
	cpu_status state;

	if (vector < 0 || vector >= NUM_IO_VECTORS)
		return B_BAD_VALUE;

	/* lock the structures down so it is not modified while we search */
	state = disable_interrupts();
#if defined(RENKU_IRQ_DRAIN_TEST) && RENKU_IRQ_DRAIN_TEST
	acquire_drain_test_vector(vector, handler, data);
#else
	acquire_spinlock(&sVectors[vector].vector_lock);
#endif

	/* loop through the available handlers and try to find a match.
	 * We go forward through the list but this means we start with the
	 * most recently added handlers.
	 */
	for (io = sVectors[vector].handler_list; io != NULL; io = io->next) {
		/* we have to match both function and data */
		if (io->func == handler && io->data == data) {
			if (last != NULL)
				last->next = io->next;
			else
				sVectors[vector].handler_list = io->next;

			// Check if we need to disable the interrupt
			if (io->use_enable_counter && --sVectors[vector].enable_count == 0)
				arch_int_disable_io_interrupt(vector);

			status = B_OK;
			break;
		}

		last = io;
	}

	if (sVectors[vector].handler_list == NULL
		&& sVectors[vector].type == INTERRUPT_TYPE_IRQ
		&& sVectors[vector].assigned_cpu != NULL
		&& sVectors[vector].assigned_cpu->handlers_count > 0) {

		int32 oldHandlersCount
			= atomic_add(&sVectors[vector].assigned_cpu->handlers_count, -1);

		if (oldHandlersCount == 1) {
			int32 oldCPU;
			SpinLocker locker;
			cpu_ent* cpu;

			do {
				locker.Unlock();

				oldCPU = sVectors[vector].assigned_cpu->cpu;

				ASSERT(oldCPU != -1);
				cpu = &gCPU[oldCPU];

				locker.SetTo(cpu->irqs_lock, false);
			} while (sVectors[vector].assigned_cpu->cpu != oldCPU);

			sVectors[vector].assigned_cpu->cpu = -1;
			cpu->irqs.Remove(sVectors[vector].assigned_cpu);
		}
	}

	release_spinlock(&sVectors[vector].vector_lock);
	restore_interrupts(state);

	// if the handler could be found and removed, we still have to free it
	if (status == B_OK)
		free(io);

	return status;
}
int main(int argc, char**)
{
 noLock = argc > 1;
 sVectors[0].no_lock_vector = noLock;
 auto* handler = static_cast<io_handler*>(std::malloc(sizeof(io_handler)));
 assert(handler);
 *handler = {callback, nullptr, nullptr, false};
 sVectors[0].handler_list = handler;
 std::thread dispatcher([] { io_interrupt_handler(0, B_EDGE_TRIGGERED); });
 wait_for(entered);
 isRemover = true;
 status_t status = remove_io_interrupt_handler(0, callback, nullptr);
 bool callbackExited = exited.load();
 std::printf("mode=%s removal=%d callback_exited_at_return=%d\n",
  noLock ? "no-lock" : "locked", status, callbackExited);
 std::fflush(stdout);
 assert(status == B_OK);
 assert(callbackExited == !noLock);
 removed.store(true);
 // In no-lock mode the real dispatcher now reads io->next after real free(io).
 dispatcher.join();
 return 0;
}
