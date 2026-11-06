#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Characterize actual kernel removal on modeled locked/no-lock vectors.

The no-lock case is an expected AddressSanitizer failure, not a passing safety
test. This does not establish that a physical GPU uses a no-lock vector.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("source", type=pathlib.Path)
parser.add_argument("--output", type=pathlib.Path, required=True)
args = parser.parse_args()
source = args.source.read_text()
license_header = re.match(r"/\*.*?\*/", source, re.S)
if license_header is None:
    raise SystemExit("Missing source license header")
functions = []
for name, result_type in (("io_interrupt_handler", "int"),
                          ("remove_io_interrupt_handler", "status_t")):
    match = re.search(r"^" + result_type + r"\n" + name
                      + r"\([^)]*\)\n\{.*?^}", source, re.M | re.S)
    if match is None:
        raise SystemExit("Missing function: " + name)
    functions.append(match.group())

prefix = r'''
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
'''
tests = r'''
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
'''
args.output.mkdir(parents=True, exist_ok=False)
environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1")
with tempfile.TemporaryDirectory(prefix="renku-irq-contract-") as directory:
    cpp = pathlib.Path(directory) / "test.cpp"
    binary = pathlib.Path(directory) / "test"
    cpp.write_text(license_header.group() + "\n// Extracted kernel functions; "
                  "host harness additions SPDX-License-Identifier: MIT\n"
                  + prefix + "\n\n".join(functions) + tests)
    (args.output / "extracted-test.cpp").write_text(cpp.read_text())
    command = ["g++", "-std=c++17", "-pthread", "-Wall", "-Wextra", "-Werror",
               "-g", "-O1", "-fsanitize=address", "-fno-omit-frame-pointer",
               "-no-pie", str(cpp), "-o", str(binary)]
    build = subprocess.run(command, capture_output=True, text=True, timeout=60)
    (args.output / "build.log").write_text(build.stdout + build.stderr)
    build.check_returncode()
    results = {}
    for mode in ("locked", "no-lock"):
        run = subprocess.run([str(binary)] + ([mode] if mode == "no-lock" else []),
                             capture_output=True, text=True, timeout=10,
                             env=environment)
        (args.output / (mode + ".log")).write_text(run.stdout + run.stderr)
        expected = (run.returncode == 0 and "callback_exited_at_return=1" in run.stdout
                    if mode == "locked" else
                    run.returncode != 0 and "callback_exited_at_return=0" in run.stdout
                    and "ERROR: AddressSanitizer: heap-use-after-free" in run.stderr
                    and "io_interrupt_handler" in run.stderr
                    and "remove_io_interrupt_handler" in run.stderr)
        results[mode] = {"returncode": run.returncode, "expected_observed": expected}
    manifest = {"scope": "host model; expected no-lock defect, not hardware evidence",
                "source": str(args.source.resolve()),
                "source_sha256": hashlib.sha256(args.source.read_bytes()).hexdigest(),
                "harness_sha256": hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
                "extracted_sha256": hashlib.sha256(cpp.read_bytes()).hexdigest(),
                "compiler": subprocess.check_output(["g++", "--version"], text=True),
                "build_command": command, "cases": results}
    (args.output / "result.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    raise SystemExit(0 if all(r["expected_observed"] for r in results.values()) else 1)
