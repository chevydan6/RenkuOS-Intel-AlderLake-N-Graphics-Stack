#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Check whether a locked dispatcher still uses a retired vector assignment.

Extract real dispatch, removal and free functions. Model OS locks/accounting;
pause at accounting entry, then attempt removal and vector release. No hardware.
"""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=pathlib.Path)
parser.add_argument("--output", type=pathlib.Path, required=True)
parser.add_argument("--debug", choices=(0, 1), type=int, default=0)
parser.add_argument("--trigger", choices=("edge", "level"), default="edge")
args = parser.parse_args()
source = args.source.read_text()
functions = []
for result, name in (("int", "io_interrupt_handler"),
                     ("status_t", "remove_io_interrupt_handler"),
                     ("void", "free_io_interrupt_vectors")):
    match = re.search(r"^" + result + r"\n" + name
                      + r"\([^)]*\)\n\{.*?^}", source, re.M | re.S)
    if match is None:
        raise SystemExit("Missing function: " + name)
    functions.append(match.group())
license_header = re.match(r"/\*.*?\*/", source, re.S)
if license_header is None:
    raise SystemExit("Missing original license")

prefix = r'''
// Host harness additions SPDX-License-Identifier: MIT
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
#define DEBUG_INTERRUPTS @DEBUG@
#define ASSERT assert
#define ASSERT_PRINT(condition, ...) assert(condition)
#define dprintf(...) ((void)0)
#define panic(...) std::abort()
struct io_handler {
 interrupt_handler func;
 void* data;
 io_handler* next;
 bool use_enable_counter;
 bool no_handled_info;
 int64_t handled_count;
};
struct Assignment { int32 handlers_count = 0; int cpu = -1; };
struct IRQList { void Remove(Assignment*) {} };
struct cpu_ent {
 std::mutex irqs_lock;
 IRQList irqs;
 bigtime_t interrupt_time = 0, irq_time = 0;
};
struct io_vector {
 bool no_lock_vector = false;
 std::mutex vector_lock, load_lock;
 io_handler* handler_list = nullptr;
 int type = INTERRUPT_TYPE_IRQ, enable_count = 0;
 Assignment* assigned_cpu = nullptr;
 bigtime_t last_measure_active = 0;
 int trigger_count = 0, handled_count = 0, unhandled_count = 0;
 int ignored_count = 0;
};
static io_vector sVectors[NUM_IO_VECTORS];
static cpu_ent gCPU[1];
static std::mutex sIOInterruptVectorAllocationLock;
static bool sAllocatedIOInterruptVectors[NUM_IO_VECTORS] = {true};
static std::atomic<bool> tailEntered{false}, removalContended{false};
static std::atomic<bool> retired{false};
static thread_local bool isDispatcher = false, isRemover = false;
static bool accountingValid = false;

static void wait_for_tail_or_retirement(bool controller)
{
 const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
 while (controller ? !tailEntered.load()
                   : !(removalContended.load() || retired.load())) {
  if (std::chrono::steady_clock::now() >= deadline) std::abort();
  std::this_thread::yield();
 }
}
static void acquire_spinlock(std::mutex* lock)
{
 if (isRemover && lock == &sVectors[0].vector_lock) {
  if (lock->try_lock()) return;
  removalContended.store(true);
 }
 lock->lock();
}
static void release_spinlock(std::mutex* lock) { lock->unlock(); }
class SpinLocker {
 std::mutex* lock = nullptr;
public:
 SpinLocker() = default;
 explicit SpinLocker(std::mutex& value)
 {
  if (isDispatcher && &value == &sVectors[0].load_lock) {
   tailEntered.store(true);
   wait_for_tail_or_retirement(false);
  }
  SetTo(value, false);
 }
 ~SpinLocker() { Unlock(); }
 void Unlock() { if (lock) { release_spinlock(lock); lock = nullptr; } }
 void SetTo(std::mutex& value, bool)
 { Unlock(); lock = &value; acquire_spinlock(lock); }
};
using InterruptsSpinLocker = SpinLocker;
using MutexLocker = SpinLocker;
static cpu_status disable_interrupts() { return 0; }
static void restore_interrupts(cpu_status) {}
static bool are_interrupts_enabled() { return false; }
static void arch_int_disable_io_interrupt(int) {}
static int32 atomic_add(int32* value, int32 delta)
{ int32 old = *value; *value += delta; return old; }
static cpu_ent* get_cpu_struct() { return &gCPU[0]; }
static void update_int_load(int vector)
{
 // Model the assignment dependency of real load accounting without crashing.
 accountingValid = sVectors[vector].assigned_cpu != nullptr
  && sAllocatedIOInterruptVectors[vector];
}
static bigtime_t system_time()
{
 return std::chrono::duration_cast<std::chrono::microseconds>(
  std::chrono::steady_clock::now().time_since_epoch()).count();
}
static int callback(void*) { return B_HANDLED_INTERRUPT; }
'''
tests = r'''
int main()
{
 Assignment assignment;
 sVectors[0].assigned_cpu = &assignment;
 auto* handler = static_cast<io_handler*>(std::malloc(sizeof(io_handler)));
 assert(handler);
 *handler = {callback, nullptr, nullptr, false, false, 0};
 sVectors[0].handler_list = handler;
 std::thread dispatcher([] {
  isDispatcher = true;
  assert(io_interrupt_handler(0, @TRIGGER@) == B_HANDLED_INTERRUPT);
 });
 wait_for_tail_or_retirement(true);
 isRemover = true;
 assert(remove_io_interrupt_handler(0, callback, nullptr) == B_OK);
 free_io_interrupt_vectors(1, 0);
 retired.store(true);
 dispatcher.join();
 assert(sVectors[0].assigned_cpu == nullptr);
 assert(!sAllocatedIOInterruptVectors[0]);
 std::printf("locked_tail accounting_valid=%d removal_contended=%d vector_freed=1\n",
  accountingValid, removalContended.load());
 return accountingValid && removalContended.load() ? 0 : 2;
}
'''
args.output.mkdir(parents=True, exist_ok=False)
with tempfile.TemporaryDirectory(prefix="renku-irq-tail-") as directory:
    cpp = pathlib.Path(directory) / "test.cpp"
    binary = pathlib.Path(directory) / "test"
    cpp.write_text(license_header.group()
                  + prefix.replace("@DEBUG@", str(args.debug))
                  + "\n\n".join(functions)
                  + tests.replace("@TRIGGER@", "B_" + args.trigger.upper() + "_TRIGGERED"))
    command = ["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
               "-g", "-O1", str(cpp), "-o", str(binary)]
    build = subprocess.run(command, capture_output=True, text=True, timeout=60)
    (args.output / "build.log").write_text(build.stdout + build.stderr)
    build.check_returncode()
    run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
    (args.output / "test.log").write_text(run.stdout + run.stderr)
    manifest = {"scope": "host model; actual dispatcher/removal/free, modeled accounting",
                "source": str(args.source.resolve()),
                "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest(),
                "harness_sha256": hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
                "generated_sha256": hashlib.sha256(cpp.read_bytes()).hexdigest(),
                "debug_interrupts": args.debug, "trigger": args.trigger,
                "build_command": command, "returncode": run.returncode}
    (args.output / "result.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(run.stdout, end="")
    raise SystemExit(run.returncode)
