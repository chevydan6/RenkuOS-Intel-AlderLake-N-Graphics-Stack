# SPDX-License-Identifier: MIT
"""Exercise production aperture-release diagnostics with a faulting GART."""
import argparse
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=pathlib.Path)
args = parser.parse_args()
source = args.source.read_text()
body = source.split('static void\nrelease_aperture(intel_info& info)\n', 1)[1].split('\n\n\nstatus_t\nintel_free_memory', 1)[0]
assert source.count('release_aperture(info);') == 3
callbacks = source[source.index('status_t\nintel_free_memory('):source.index('status_t\nintel_extreme_init(')]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cerrno>
using status_t = int32_t;
using addr_t = uintptr_t;
using phys_addr_t = uint64_t;
using uint32 = uint32_t;
constexpr status_t B_OK = 0;
#define B_PRId32 "d"
static char message[512];
static unsigned logs, calls;
static status_t result;
static int32_t lastID;
static void log_error(const char* format, ...) {
 logs++; va_list args; va_start(args, format);
 vsnprintf(message, sizeof(message), format, args); va_end(args);
}
#define ERROR log_error
static unsigned ownerFinishes, ownerReleases, modulePuts;
static unsigned ownerAllocations, ownerFrees, gartAllocations, gartFrees;
static bool lastFinish;
static status_t finishResult, releaseResult;
#define INTEL_MEMORY_OWNER_MODULE_NAME "test-owner"
static void put_module(const char*) { modulePuts++; }
struct Owner {
 status_t allocate(uint64_t lease, size_t size, size_t alignment,
  uint32 flags, addr_t* base, phys_addr_t* physical) {
  assert(lease == 91 && size == 4096 && alignment == 4096 && flags == 3);
  ownerAllocations++;
  if (result == B_OK) { *base = 0x10000; *physical = 0x20000; }
  return result;
 }
 status_t free(uint64_t lease, addr_t base) {
  assert(lease == 91 && base == 0x10000); ownerFrees++; return result;
 }
 status_t finish_display(uint64_t lease, bool success) {
  assert(lease == 91); ownerFinishes++; lastFinish = success; return finishResult;
 }
 status_t release_display(uint64_t lease) {
  assert(lease == 91); ownerReleases++; return releaseResult;
 }
};
struct intel_info {
 int32_t aperture;
 Owner* memory_owner;
 uint64_t memory_owner_lease;
 bool memory_owner_ready;
};
struct Gart {
 status_t allocate_memory(int32_t id, size_t size, size_t alignment,
  uint32 flags, addr_t* base, phys_addr_t* physical) {
  assert(id == 73 && size == 4096 && alignment == 4096 && flags == 3);
  gartAllocations++;
  if (result == B_OK) { *base = 0x10000; *physical = 0x20000; }
  return result;
 }
 status_t free_memory(int32_t id, addr_t base) {
  assert(id == 73 && base == 0x10000); gartFrees++; return result;
 }
 status_t unmap_aperture(int32_t id) {calls++; lastID=id; return result;}
};
static Gart backend;
static Gart* gGART = &backend;
static void release_aperture(intel_info& info)
''' + body + callbacks + r'''
int main() {
 intel_info info{}; info.aperture = 73;
 for (status_t status : {0, -1, -2, -5, -12}) {
  result=status; logs=calls=0; message[0]=0;
  release_aperture(info);
  assert(calls==1 && lastID==73 && info.aperture==73);
  assert(logs==(status<0 ? 1u : 0u));
  if(status<0) {
   assert(strstr(message,"aperture 73")!=nullptr);
   assert(strstr(message,strerror(status))!=nullptr);
   assert(strstr(message,"retained by GART")!=nullptr);
  }
 }
 Owner owner;
 for (bool initialized : {false, true}) {
  for (status_t failure : {0, -5}) {
   info.memory_owner = &owner;
   info.memory_owner_lease = 91;
   info.memory_owner_ready = initialized;
   finishResult = releaseResult = failure;
   ownerFinishes = ownerReleases = modulePuts = calls = logs = 0;
   release_aperture(info);
   assert(calls == 0 && ownerReleases == 1 && modulePuts == 1);
   assert(ownerFinishes == (initialized ? 0u : 1u));
   if (!initialized) assert(!lastFinish);
   assert(info.memory_owner == nullptr && info.memory_owner_lease == 0);
   assert(!info.memory_owner_ready);
   assert(logs == (failure < 0 ? (initialized ? 1u : 2u) : 0u));
  }
 }
 for (bool owned : {false, true}) {
  for (status_t status : {0, -5}) {
   info.memory_owner = owned ? &owner : nullptr;
   info.memory_owner_lease = owned ? 91 : 0;
   ownerAllocations = ownerFrees = gartAllocations = gartFrees = 0;
   result = status;
   addr_t base = 73;
   phys_addr_t physical = 91;
   assert(intel_allocate_memory(info, 4096, 4096, 3, &base, &physical) == status);
   assert(base == (status == B_OK ? 0x10000u : 73u));
   assert(physical == (status == B_OK ? 0x20000u : 91u));
   assert(intel_free_memory(info, 0x10000) == status);
   assert(ownerAllocations == (owned ? 1u : 0u) && ownerFrees == ownerAllocations);
   assert(gartAllocations == (owned ? 0u : 1u) && gartFrees == gartAllocations);
  }
 }
 puts("4 actual allocation/free routing cases pass; owner and legacy paths");
 puts("4 retained-owner init/release cases pass; no direct GART teardown");
 puts("5 production aperture-release success/failure diagnostic cases passed; all 3 call sites checked");
}
'''
fixture = '#include <initializer_list>\n' + fixture
with tempfile.TemporaryDirectory(prefix='renku-cleanup-') as directory:
    cpp = pathlib.Path(directory) / 'test.cpp'
    binary = pathlib.Path(directory) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
