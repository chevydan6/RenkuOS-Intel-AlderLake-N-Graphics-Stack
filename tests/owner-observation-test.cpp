// SPDX-License-Identifier: MIT
// Actual owner observation functions; mocked PCI, kernel mutex and module API.
#include "IntelApertureLeaseState.h"
#include "IntelN100PciPower.h"
#include "IntelN100GtAck.h"
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <mutex>
#include <thread>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using status_t = int;
using addr_t = uintptr_t;
#ifndef B_OK
const int B_OK = 0, B_BUSY = -1, B_NO_MEMORY = -2, B_BAD_VALUE = -3, B_ERROR = -4;
#endif
using mutex = std::mutex;
static void mutex_lock(mutex* m) { m->lock(); }
static void mutex_unlock(mutex* m) { m->unlock(); }
struct pci_module_info {
	uint32 (*read_pci_config)(uint8, uint8, uint8, uint16, uint8);
};
static mutex sLock;
static IntelApertureLeaseState sState;
static pci_module_info* sPCI;
static struct { int registers_area; addr_t registers; } sResources;
static uint32 sMockRegisters[0x140000 / sizeof(uint32)];
static std::mutex sAckLock;
static std::condition_variable sAckChanged;
static bool sAckGate, sAckEntered, sAckProceed, sChangeAck, sSecondInvalid;
static unsigned sBarriers;
static void memory_read_barrier()
{
	assert(sLock.try_lock()); sLock.unlock();
	std::unique_lock<std::mutex> lock(sAckLock);
	unsigned barrier = sBarriers++;
	if (barrier == 0 && sAckGate) {
		sAckEntered = true; sAckChanged.notify_all();
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sAckProceed; }));
	}
	if (barrier == 1 && sChangeAck)
		sMockRegisters[kIntelN100GtAckOffset / 4] = 0x8001;
	if (barrier == 1 && sSecondInvalid)
		sMockRegisters[kIntelN100GtAckOffset / 4] = UINT32_MAX;
}
#define dprintf(...) ((void)0)
#include "observation-under-test.inc"
#undef dprintf

using Owner = IntelApertureLeaseState;
static std::mutex sConfigLock;
static std::condition_variable sChanged;
static bool sGate, sEntered, sProceed;
static uint32 sPMControl;
static unsigned sReads;
static uint64 sDisplay;

static uint32 Read(uint8 bus, uint8 device, uint8 function, uint16 offset, uint8 size)
{
	assert(bus == 0 && device == 2 && function == 0);
	assert(sLock.try_lock()); // The actual adapter must drop its lock for PCI.
	sLock.unlock();
	std::unique_lock<std::mutex> lock(sConfigLock);
	sReads++;
	if (sGate) {
		sEntered = true;
		sChanged.notify_all();
		assert(sChanged.wait_for(lock, std::chrono::seconds(5), [] { return sProceed; }));
	}
	switch (offset) {
		case 0: assert(size == 4); return 0x46d18086;
		case 8: assert(size == 4); return 0x03000000;
		case 0x0e: assert(size == 1); return 0;
		case 4: assert(size == 2); return 2;
		case 6: assert(size == 2); return 0x10;
		case 0x10: assert(size == 4); return 4;
		case 0x14: assert(size == 4); return 0x60;
		case 0x34: assert(size == 1); return 0x40;
		case 0x40: assert(size == 2); return 1;
		case 0x42: assert(size == 2); return 3;
		case 0x44: assert(size == 2); return sPMControl;
	}
	assert(false);
	return UINT32_MAX;
}

static void Reset()
{
	// No workers exist here. These are mock records, never a real owner reset.
	sState.~Owner(); new (&sState) Owner();
	sObservationBlocked = false;
	sObservationPhase = ObservationIdle;
	sObservationLease = 0;
	sDevice = {0, 2, 0, 0x6000000000ULL, 0x1000000};
	sRegistersUncached = true;
	sResources.registers_area = 123;
	sResources.registers = (addr_t)sMockRegisters;
	sMockRegisters[kIntelN100GtAckOffset / 4] = 0;
	sAckGate = sAckEntered = sAckProceed = sChangeAck = sSecondInvalid = false;
	sBarriers = 0;
	sPMControl = 0;
	sGate = sEntered = sProceed = false;
	sReads = 0;
	assert(sState.BeginInitialization(sDisplay) == Owner::Success);
	assert(sState.FinishInitialization(sDisplay, true) == Owner::Success);
}

static void Wait()
{
	std::unique_lock<std::mutex> lock(sConfigLock);
	assert(sChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEntered; }));
}

static void Proceed()
{
	std::lock_guard<std::mutex> lock(sConfigLock);
	sProceed = true;
	sChanged.notify_all();
}

static void ReleaseDisplay()
{
	std::lock_guard<std::mutex> lock(sLock);
	assert(sState.Release(sDisplay) == Owner::Success);
}

int main()
{
	pci_module_info pci = {Read}; sPCI = &pci;
	IntelN100PciSnapshot snapshot = {};
	uint64 lease = 0;
	unsigned cases = 0;
	Reset();
	assert(BeginObservation(NULL, &snapshot) == B_BAD_VALUE);
	assert(BeginObservation(&lease, NULL) == B_BAD_VALUE && sReads == 0); cases++;
	assert(BeginObservation(&lease, &snapshot) == B_OK);
	uint64 first = lease, untouched = 0xfeed;
	assert(BeginObservation(&untouched, &snapshot) == B_BUSY && untouched == 0xfeed);
	assert(EndObservation(0) == B_BAD_VALUE);
	assert(EndObservation(lease + 1) == B_BAD_VALUE);
	assert(EndObservation(lease) == B_OK);
	assert(EndObservation(lease) == B_BAD_VALUE);
	assert(BeginObservation(&lease, &snapshot) == B_OK && lease > first);
	assert(EndObservation(lease) == B_OK); cases++;
	// A different existing Request user blocks admission without poisoning it.
	Reset(); uint64 request = 0;
	assert(sState.AcquireRequest(request) == Owner::Success);
	assert(BeginObservation(&lease, &snapshot) == B_BUSY && sReads == 0);
	assert(sState.Release(request) == Owner::Success);
	assert(BeginObservation(&lease, &snapshot) == B_OK);
	assert(EndObservation(lease) == B_OK); cases++;
	Reset(); sPMControl = 3; lease = 0xfeed;
	IntelN100PciSnapshot before = snapshot;
	assert(BeginObservation(&lease, &snapshot) == B_ERROR && lease == 0xfeed);
	assert(IntelN100PciPower::Same(before, snapshot));
	assert(sObservationBlocked && sObservationPhase == ObservationIdle);
	unsigned reads = sReads;
	assert(BeginObservation(&lease, &snapshot) == B_BUSY && reads == sReads);
	ReleaseDisplay();
	assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	Reset();
	assert(BeginObservation(&lease, &snapshot) == B_OK);
	sPMControl = 3;
	assert(EndObservation(lease) == B_ERROR && sObservationBlocked);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	// Deterministic close during admission: pin persists until PCI returns.
	Reset(); sGate = true; lease = 0xfeed; before = snapshot;
	status_t result = 42;
	std::thread admission([&] { result = BeginObservation(&lease, &snapshot); });
	Wait(); ReleaseDisplay();
	assert(sState.BeginCleanup(true, true) == Owner::Busy);
	Proceed(); admission.join();
	assert(result == B_ERROR && lease == 0xfeed && sObservationBlocked);
	assert(IntelN100PciPower::Same(before, snapshot));
	assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	Reset(); assert(BeginObservation(&lease, &snapshot) == B_OK);
	ReleaseDisplay(); assert(EndObservation(lease) == B_ERROR);
	assert(sObservationBlocked && sState.BeginCleanup(true, true) == Owner::Success); cases++;
	// Two End calls cannot consume the same retained operation pin.
	Reset(); assert(BeginObservation(&lease, &snapshot) == B_OK);
	sGate = true;
	std::thread retirement([&] { result = EndObservation(lease); });
	Wait();
	assert(EndObservation(lease) == B_BAD_VALUE);
	assert(BeginObservation(&untouched, &snapshot) == B_BUSY);
	Proceed(); retirement.join();
	assert(result == B_OK && !sObservationBlocked);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	// Display close during final PCI recheck cannot retire the pin early.
	Reset(); assert(BeginObservation(&lease, &snapshot) == B_OK);
	sGate = true;
	std::thread closing([&] { result = EndObservation(lease); });
	Wait(); ReleaseDisplay();
	assert(sState.BeginCleanup(true, true) == Owner::Busy);
	Proceed(); closing.join();
	assert(result == B_ERROR && sObservationBlocked);
	assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	// Failed concurrent GART backing operations revoke diagnostic admission.
	Reset(); assert(BeginObservation(&lease, &snapshot) == B_OK);
	sGate = true;
	std::thread failedBacking([&] { result = EndObservation(lease); });
	Wait();
	{
		std::lock_guard<std::mutex> lock(sLock);
		Owner::Operation operation;
		assert(sState.BeginOperation(sDisplay, Owner::Allocate, operation) == Owner::Success);
		assert(sState.EndOperation(operation, true) == Owner::Success);
	}
	Proceed(); failedBacking.join();
	assert(result == B_ERROR && sObservationBlocked);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	// Actual volatile loads target mock RAM, never hardware in this harness.
	Reset(); IntelN100GtAckSample ack = {0xfeed, 0xbeef};
	assert(ObserveGtAck(NULL) == B_BAD_VALUE && sReads == 0);
	assert(ObserveGtAck(&ack) == B_OK && ack.first == 0 && ack.second == 0);
	assert(sBarriers == 4 && sObservationPhase == ObservationIdle); cases++;
	Reset(); sChangeAck = true;
	assert(ObserveGtAck(&ack) == B_OK && ack.first == 0 && ack.second == 0x8001);
	assert(sBarriers == 4); cases++;
	Reset(); sDevice.size = 0x131000;
	assert(ObserveGtAck(&ack) == B_OK && sBarriers == 4); cases++;
	for (unsigned invalid = 0; invalid < 7; invalid++) {
		Reset(); ack = {0xfeed, 0xbeef};
		switch (invalid) {
			case 0: sRegistersUncached = false; break;
			case 1: sResources.registers_area = -1; break;
			case 2: sResources.registers = 0; break;
			case 3: sResources.registers++; break;
			case 4: sDevice.size = 0x130000; break;
			case 5: sResources.registers = UINTPTR_MAX - 3; break;
			case 6: sResources.registers = UINTPTR_MAX - kIntelN100GtAckOffset; break;
		}
		assert(ObserveGtAck(&ack) == B_ERROR && sBarriers == 0);
		assert(ack.first == 0xfeed && ack.second == 0xbeef);
		assert(sObservationBlocked && sObservationPhase == ObservationIdle);
		ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	}
	Reset(); ack = {0xfeed, 0xbeef}; sMockRegisters[kIntelN100GtAckOffset / 4] = UINT32_MAX;
	assert(ObserveGtAck(&ack) == B_ERROR && sBarriers == 2);
	assert(ack.first == 0xfeed && ack.second == 0xbeef && sObservationBlocked);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	Reset(); ack = {0xfeed, 0xbeef}; sSecondInvalid = true;
	assert(ObserveGtAck(&ack) == B_ERROR && sBarriers == 4);
	assert(ack.first == 0xfeed && ack.second == 0xbeef && sObservationBlocked);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	// A v1 End must not consume v2's Reading pin while its read is gated.
	Reset(); sAckGate = true; ack = {0xfeed, 0xbeef};
	std::thread reading([&] { result = ObserveGtAck(&ack); });
	{
		std::unique_lock<std::mutex> lock(sAckLock);
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sAckEntered; }));
	}
	assert(EndObservation(sObservationLease) == B_BAD_VALUE);
	assert(BeginObservation(&untouched, &snapshot) == B_BUSY);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy);
	{
		std::lock_guard<std::mutex> lock(sAckLock);
		sAckProceed = true; sAckChanged.notify_all();
	}
	reading.join();
	assert(result == B_ERROR && ack.first == 0xfeed && ack.second == 0xbeef);
	assert(sObservationBlocked && sState.BeginCleanup(true, true) == Owner::Success); cases++;
	for (unsigned failure = 0; failure < 2; failure++) {
		Reset(); sAckGate = true; ack = {0xfeed, 0xbeef};
		std::thread uncertain([&] { result = ObserveGtAck(&ack); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sAckEntered; }));
		}
		if (failure == 0) {
			std::lock_guard<std::mutex> lock(sConfigLock);
			sPMControl = 3;
		} else {
			std::lock_guard<std::mutex> lock(sLock);
			Owner::Operation operation;
			assert(sState.BeginOperation(sDisplay, Owner::Free, operation) == Owner::Success);
			assert(sState.EndOperation(operation, true) == Owner::Success);
		}
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			sAckProceed = true; sAckChanged.notify_all();
		}
		uncertain.join();
		assert(result == B_ERROR && ack.first == 0xfeed && ack.second == 0xbeef);
		assert(sObservationBlocked && sObservationPhase == ObservationIdle);
		ReleaseDisplay();
		assert(sState.BeginCleanup(true, true) == (failure ? Owner::Busy : Owner::Success)); cases++;
	}
	Reset(); ack = {0xfeed, 0xbeef}; sPMControl = 3;
	assert(ObserveGtAck(&ack) == B_ERROR && sBarriers == 0);
	assert(ack.first == 0xfeed && ack.second == 0xbeef); cases++;
	printf("%u actual owner observation cases passed; mocked PCI/kernel only.\n", cases);
}
