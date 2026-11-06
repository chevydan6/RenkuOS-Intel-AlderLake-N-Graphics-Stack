#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise production query/allocate/free versus unmap serialization with real host threads."""
import argparse
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
args = parser.parse_args()
source = args.source.read_text()
helper = re.search(r'^Aperture \*\nget_aperture\([^)]*\)\n\{.*?^}', source, re.M | re.S).group()
functions = []
for name in ('get_aperture_info', 'allocate_memory', 'free_memory', 'unmap_aperture'):
    match = re.search(r'^static status_t\n' + name + r'\([^)]*\)\n\{.*?^}', source, re.M | re.S)
    if match is None:
        raise SystemExit('missing production function: ' + name)
    functions.append(match.group())
prefix = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
using status_t = int;
using aperture_id = int;
using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
using uint32 = uint32_t;
constexpr size_t B_PAGE_SIZE = 4096;
constexpr uint32 B_APERTURE_NEED_PHYSICAL = 2, APERTURE_PUBLIC_FLAGS_MASK = 3;
constexpr int B_BAD_VALUE = -1, B_NO_MEMORY = -3, B_BUSY = -16;
#define ROUNDUP(x,a) (((x)+(a)-1)&~((a)-1))
#define ERROR(...) ((void)0)
struct Page { uint64_t physical_page_number = 32; };
struct aperture_memory { Page* page; addr_t base; };
constexpr int B_OK = 0, B_ENTRY_NOT_FOUND = -2;
struct aperture_info { int value; };
static std::mutex gate;
static std::condition_variable changed;
static bool entered, releaseQuery, teardownBlocked, destroyed;
static thread_local bool teardownThread;
struct TestLock { std::mutex mutex; };
static TestLock sLock;
struct Autolock {
 TestLock& lock;
 explicit Autolock(TestLock& l):lock(l) {
  if (teardownThread && &l == &sLock) {
   if (!l.mutex.try_lock()) {
    { std::lock_guard<std::mutex> guard(gate); teardownBlocked = true; }
    changed.notify_all(); l.mutex.lock();
   }
  } else l.mutex.lock();
 }
 ~Autolock() { lock.mutex.unlock(); }
};
// The backend and page allocator are modeled. Pause in the selected backend
// method while production callbacks must retain the manager lookup lock.
static void PauseOperation() {
 std::unique_lock<std::mutex> guard(gate);
 entered = true; changed.notify_all();
 assert(changed.wait_for(guard,std::chrono::seconds(10),[]{return releaseQuery;}));
 assert(!destroyed);
}
struct Aperture {
 TestLock lock;
 Page page;
 aperture_memory memory{&page, 0x10000};
 bool live = false;
 TestLock& Lock() { return lock; }
 bool IsCleanupPending() const { return false; }
 int GetInfo(aperture_info* info) {
  PauseOperation(); info->value = 73; return B_OK;
 }
 aperture_memory* CreateMemory(size_t size, size_t, uint32) {
  PauseOperation(); assert(size == B_PAGE_SIZE && !live);
  live = true; return &memory;
 }
 int AllocateMemory(aperture_memory* item, uint32) {
  assert(item == &memory && live); return B_OK;
 }
 int BindMemory(aperture_memory* item, addr_t, size_t) {
  assert(item == &memory && live); return B_OK;
 }
 aperture_memory* GetMemory(addr_t base) {
  PauseOperation(); return live && base == memory.base ? &memory : nullptr;
 }
 int DeleteMemory(aperture_memory* item) {
  assert(item == &memory && live); live = false; return B_OK;
 }
 int DeleteAllMemory() { live = false; return B_OK; }
 void MarkCleanupPending() {}
 ~Aperture() {
  std::lock_guard<std::mutex> guard(gate); assert(!live); destroyed = true;
 }
};
struct Registry {
 Aperture* value = nullptr;
 Aperture* Lookup(int id) { return id == 1 ? value : nullptr; }
 void Remove(Aperture* item) { assert(value == item); value = nullptr; }
};
static Registry sApertureHashTable;

'''
suffix = r'''
int main() {
 for (unsigned operation = 0; operation < 3; operation++) {
 for (unsigned round = 0; round < 100; round++) {
  entered = releaseQuery = teardownBlocked = destroyed = false;
  sApertureHashTable.value = new Aperture;
  sApertureHashTable.value->live = operation == 2;
  addr_t base = 0; phys_addr_t physical = 0;
  aperture_info info{}; int queryResult = -99, teardownResult = -99;
  std::thread reader([&]{
   if (operation == 0) queryResult = get_aperture_info(1,&info);
   else if (operation == 1)
    queryResult = allocate_memory(1,4096,4096,B_APERTURE_NEED_PHYSICAL,
      &base,&physical);
   else queryResult = free_memory(1,0x10000);
  });
  { std::unique_lock<std::mutex> guard(gate);
    assert(changed.wait_for(guard,std::chrono::seconds(5),[]{return entered;})); }
  std::thread teardown([&]{ teardownThread = true; teardownResult = unmap_aperture(1); });
  { std::unique_lock<std::mutex> guard(gate);
    assert(changed.wait_for(guard,std::chrono::seconds(5),[]{return teardownBlocked;}));
    assert(!destroyed); releaseQuery = true; }
  changed.notify_all(); reader.join(); teardown.join();
  assert(queryResult == B_OK && teardownResult == B_OK && destroyed);
  if (operation == 0) assert(info.value == 73);
  if (operation == 1) assert(base == 0x10000 && physical == 32 * B_PAGE_SIZE);
  assert(get_aperture_info(1,&info) == B_ENTRY_NOT_FOUND);
  assert(unmap_aperture(1) == B_ENTRY_NOT_FOUND);
  assert(allocate_memory(1,4096,4096,0,&base,nullptr) == B_ENTRY_NOT_FOUND);
  assert(free_memory(1,0x10000) == B_ENTRY_NOT_FOUND);
 }
 }
 puts("300 threaded production query/allocate/free versus unmap rounds passed");
}
'''
with tempfile.TemporaryDirectory(prefix='gart-threaded-') as directory:
    tmp = pathlib.Path(directory)
    (tmp/'test.cpp').write_text(prefix + helper + '\n'.join(functions) + suffix)
    subprocess.run(['g++', '-std=c++17', '-pthread', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', str(tmp/'test.cpp'), '-o', str(tmp/'test')], check=True)
    subprocess.run([str(tmp/'test')], check=True, timeout=40)
