// SPDX-License-Identifier: MIT
// Actual owner observation functions; mocked PCI, kernel mutex and module API.
#include "IntelApertureLeaseState.h"
#include "IntelN100PciPower.h"
#include "IntelN100GtAck.h"
#include "IntelN100Forcewake.h"
#include "IntelN100BcsInventory.h"
#include "IntelN100ForcewakeGroup.h"
#if __has_include("IntelN100BcsReset.h")
#include "IntelN100BcsReset.h"
#endif
#if __has_include("IntelN100ResetSafety.h")
#include "IntelN100ResetSafety.h"
#endif
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <initializer_list>
#include <new>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mutex>
#include <thread>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using status_t = int;
using addr_t = uintptr_t;
using bigtime_t = int64_t;
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
static uint64_t sClock;
static unsigned sRequestWrites;
static bool sNoSet, sNoClear, sBadPostRead;
static bool sWriteGate, sWriteEntered, sWriteProceed, sFailFinalOwner;
static unsigned sPostClearIdentities;
static bool sEngineGate, sEngineEntered, sEngineProceed;
static bool sTestingGroup, sGroupGate, sGroupEntered, sGroupProceed;
static unsigned sGroupWrites;
static int sGroupNoSet, sGroupNoClear;
static bool sGroupBadRead, sGroupLoseAck, sFailGroupFinalOwner;
static void MockGroupBarrier();
#ifdef INTEL_N100_BCS_RESET_H
static bool sTestingReset, sResetNoReady, sResetNoDomain, sResetLoseAck;
static bool sResetGate, sResetEntered, sResetProceed;
static unsigned sResetWrites;
static uint32 sResetFaultAck;
static bool MockResetActive();
static void MockResetBarrier();
#endif
static int64_t MockSystemTime() { return sClock; }
static int MockSnooze(int64_t delay) { sClock += delay; return 0; }
static void MockWriteBarrier() { assert(sLock.try_lock()); sLock.unlock(); }
static void MockFullBarrier()
{
	assert(sLock.try_lock()); sLock.unlock();
#ifdef INTEL_N100_BCS_RESET_H
	if (MockResetActive()) {
		MockResetBarrier();
		return;
	}
#endif
	if (sTestingGroup && sRequestWrites >= 2) {
		MockGroupBarrier();
		return;
	}
	uint32 word = sMockRegisters[0xa188 / 4];
	assert(word == (sRequestWrites == 0 ? 0x10001 : 0x10000));
	sRequestWrites++;
	sMockRegisters[kIntelN100GtAckOffset / 4] = sBadPostRead ? UINT32_MAX
		: (word == 0x10001 ? !sNoSet : sNoClear);
	if (sWriteGate && sRequestWrites == 1) {
		std::unique_lock<std::mutex> lock(sAckLock);
		sWriteEntered = true; sAckChanged.notify_all();
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sWriteProceed; }));
	}
}
static void memory_read_barrier()
{
	assert(sLock.try_lock()); sLock.unlock();
	std::unique_lock<std::mutex> lock(sAckLock);
	unsigned barrier = sBarriers++;
	if (sEngineGate && barrier == 6) {
		sEngineEntered = true; sAckChanged.notify_all();
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEngineProceed; }));
	}
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
#define system_time MockSystemTime
#define snooze MockSnooze
#define memory_write_barrier MockWriteBarrier
#define memory_full_barrier MockFullBarrier
#include "observation-under-test.inc"
#undef dprintf
#undef system_time
#undef snooze
#undef memory_write_barrier
#undef memory_full_barrier

static void MockGroupBarrier()
{
	const auto& report = sGroup.LastReport();
	const auto& domain = report.domains[report.domain];
	assert(domain.ownership == IntelN100ForcewakeGroup::EnableAttempted
		|| domain.ownership == IntelN100ForcewakeGroup::ClearAttempted);
	uint32 word = sMockRegisters[domain.request / 4];
	assert(word == (domain.ownership == IntelN100ForcewakeGroup::EnableAttempted ? 0x10001u : 0x10000u));
	sRequestWrites++; sGroupWrites++;
	if (word == 0x10001 && int(domain.kind) != sGroupNoSet)
		sMockRegisters[domain.ack / 4] = 1;
	if (word == 0x10000 && int(domain.kind) != sGroupNoClear)
		sMockRegisters[domain.ack / 4] = 0;
	if (sGroupBadRead) sMockRegisters[domain.ack / 4] = UINT32_MAX;
	if (sGroupLoseAck && domain.kind != 0) sMockRegisters[kIntelN100GtAckOffset / 4] = 0;
	if (sGroupGate && sGroupWrites == 1) {
		std::unique_lock<std::mutex> lock(sAckLock);
		sGroupEntered = true; sAckChanged.notify_all();
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sGroupProceed; }));
	}
}

using Owner = IntelApertureLeaseState;
#ifdef INTEL_N100_BCS_RESET_H
static bool MockResetActive()
{
	return sTestingReset && sGroup.LastReport().stage == IntelN100ForcewakeGroup::Work;
}
static void MockResetBarrier()
{
	using Reset = IntelN100BcsReset;
	const auto& r = sBcsReset.LastReport();
	const uint32 offsets[] = {Reset::kMode, Reset::kPrefetch, Reset::kResetControl,
		Reset::kDomain, Reset::kDomain, Reset::kResetControl};
	const uint32 words[] = {0x01000100, 0x04000400, 0x10001, 4, 4, 0x10000};
	assert(sResetWrites < 6 && r.attemptedOffset == offsets[sResetWrites]
		&& r.attemptedValue == words[sResetWrites]);
	assert(sMockRegisters[r.attemptedOffset/4] == words[sResetWrites]);
	sResetWrites++; sRequestWrites++;
	assert(r.writes == sResetWrites);
	if (r.attemptedOffset == Reset::kMode) sMockRegisters[Reset::kMode/4] = 0x300;
	if (r.attemptedOffset == Reset::kPrefetch) sMockRegisters[Reset::kPrefetch/4] = 0x400;
	if (r.attemptedOffset == Reset::kResetControl)
		sMockRegisters[Reset::kResetControl/4] = r.attemptedValue == 0x10000 ? 0 : sResetNoReady ? 1 : 3;
	if (r.attemptedOffset == Reset::kDomain && !sResetNoDomain)
		sMockRegisters[Reset::kDomain/4] = 0;
	if (sResetLoseAck) sMockRegisters[0xd84/4] = sResetFaultAck;
	if (sResetGate && sResetWrites == 1) {
		std::unique_lock<std::mutex> lock(sAckLock);
		sResetEntered = true; sAckChanged.notify_all();
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sResetProceed; }));
	}
}
#endif
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
		case 0:
			assert(size == 4);
			if (sFailGroupFinalOwner
				&& sGroup.LastReport().stage == IntelN100ForcewakeGroup::Complete)
				return UINT32_MAX;
			// Four identities per completed capture; fail final owner capture,
			// after the helper's post-clear PCI check has succeeded.
			if (sFailFinalOwner && sRequestWrites == 2 && ++sPostClearIdentities == 5)
				return UINT32_MAX;
			return 0x46d18086;
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
	sObservationPin.~Operation(); new (&sObservationPin) Owner::Operation();
	sForcewake.~IntelN100Forcewake(); new (&sForcewake) IntelN100Forcewake();
	sGroup.~IntelN100ForcewakeGroup(); new (&sGroup) IntelN100ForcewakeGroup();
	sGroupReport = {}; sGroupAttempted = false;
	sGroupBootstrapReport = {};
#ifdef INTEL_N100_BCS_RESET_H
	sBcsReset.~IntelN100BcsReset(); new (&sBcsReset) IntelN100BcsReset();
	sResetReport = {};
#ifdef INTEL_N100_RESET_SAFETY_H
	sResetSafety = {};
#endif
	sTestingReset = sResetNoReady = sResetNoDomain = sResetLoseAck = false;
	sResetGate = sResetEntered = sResetProceed = false;
	sResetWrites = 0;
	sResetFaultAck = 0;
#endif
	sHandshakeReport = {};
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
	sClock = 1000; sRequestWrites = 0;
	sNoSet = sNoClear = sBadPostRead = false;
	sWriteGate = sWriteEntered = sWriteProceed = sFailFinalOwner = false;
	sPostClearIdentities = 0;
	sEngineGate = sEngineEntered = sEngineProceed = false;
	sTestingGroup = sGroupGate = sGroupEntered = sGroupProceed = false;
	sGroupWrites = 0; sGroupNoSet = sGroupNoClear = -1;
	sGroupBadRead = sGroupLoseAck = sFailGroupFinalOwner = false;
	for (unsigned offset : {0xd84u,0xd50u,0xd58u,0xd70u}) sMockRegisters[offset/4] = 0;
	for (unsigned i = 0; i < 5; i++) sMockRegisters[kIntelN100BcsOffsets[i] / 4] = i + 1;
	sMockRegisters[0x9140 / 4] = 0x00010004;
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
	Reset(); IntelN100Forcewake::Report report = {};
	assert(RunHandshake(NULL) == B_BAD_VALUE && sRequestWrites == 0);
	assert(RunHandshake(&report) == B_OK && report.ownership == IntelN100Forcewake::Released);
	assert(report.error == 0 && report.writes == 2 && sRequestWrites == 2);
	assert(sObservationPhase == ObservationIdle && !sObservationBlocked); cases++;
	for (unsigned failure = 0; failure < 3; failure++) {
		Reset();
		sNoSet = failure == 0; sNoClear = failure == 1; sBadPostRead = failure == 2;
		status_t outcome = RunHandshake(&report);
		if (outcome != B_ERROR || report.ownership != IntelN100Forcewake::Unknown)
			fprintf(stderr, "failure=%u status=%d error=%u ownership=%u writes=%u reads=%u clock=%llu\n",
				failure, outcome, report.error, report.ownership, report.writes, report.reads,
				(unsigned long long)sClock);
		assert(outcome == B_ERROR && report.ownership == IntelN100Forcewake::Unknown);
		assert(sObservationPhase == ObservationUnknown && sObservationBlocked);
		unsigned writes = sRequestWrites, reads = sReads;
		assert(RunHandshake(&report) == B_BUSY && writes == sRequestWrites && reads == sReads);
		assert(EndObservation(sObservationLease) == B_BAD_VALUE);
		ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	Reset(); sMockRegisters[kIntelN100GtAckOffset / 4] = 1;
	assert(RunHandshake(&report) == B_ERROR && report.ownership == IntelN100Forcewake::NoRequest);
	assert(sRequestWrites == 0 && sObservationPhase == ObservationIdle);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	Reset(); sRegistersUncached = false;
	assert(RunHandshake(&report) == B_ERROR && report.error == IntelN100Forcewake::InvalidMapping);
	assert(report.ownership == IntelN100Forcewake::NoRequest && sRequestWrites == 0);
	assert(RunHandshake(&report) == B_BUSY && report.error == IntelN100Forcewake::InvalidMapping);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Success); cases++;
	Reset(); sFailFinalOwner = true;
	assert(RunHandshake(&report) == B_ERROR && report.ownership == IntelN100Forcewake::Unknown);
	assert(report.stage == IntelN100Forcewake::OwnerValidation);
	assert(report.error == IntelN100Forcewake::PowerChanged && report.cleanup == report.error);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	for (unsigned failure = 0; failure < 4; failure++) {
		Reset(); sWriteGate = true;
		std::thread handshake([&] { result = RunHandshake(&report); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sWriteEntered; }));
		}
		assert(EndObservation(sObservationLease) == B_BAD_VALUE);
		assert(BeginObservation(&untouched, &snapshot) == B_BUSY);
		IntelN100Forcewake::Report ignored = {};
		assert(RunHandshake(&ignored) == B_BUSY);
		if (failure == 3) {
			std::lock_guard<std::mutex> lock(sLock);
			Owner::Operation operation;
			assert(sState.BeginOperation(sDisplay, Owner::Free, operation) == Owner::Success);
			assert(sState.EndOperation(operation, true) == Owner::Success);
		} else ReleaseDisplay();
		if (failure == 2) {
			std::lock_guard<std::mutex> lock(sConfigLock);
			sPMControl = 3;
		}
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			if (failure == 1) sMockRegisters[kIntelN100GtAckOffset / 4] = UINT32_MAX;
			sWriteProceed = true; sAckChanged.notify_all();
		}
		handshake.join();
		assert(result == B_ERROR && report.ownership == IntelN100Forcewake::Unknown);
		assert(sRequestWrites == (failure == 1 || failure == 2 ? 1u : 2u));
		assert(sObservationPhase == ObservationUnknown);
		if (failure == 3) ReleaseDisplay();
		assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	Reset(); IntelN100BcsInventory bcs = {}, saved = {};
	assert(RunBcsInventory(NULL, &bcs) == B_BAD_VALUE);
	assert(RunBcsInventory(&report, NULL) == B_BAD_VALUE && sRequestWrites == 0);
	assert(RunBcsInventory(&report, &bcs) == B_OK && report.ownership == IntelN100Forcewake::Released);
	assert(bcs.tail == 1 && bcs.head == 2 && bcs.control == 3 && bcs.mode == 4 && bcs.pendingWake == 5);
	assert(sRequestWrites == 2 && sObservationPhase == ObservationIdle); cases++;
	for (unsigned invalid = 0; invalid < 5; invalid++) {
		Reset(); memset(&bcs, 0xa5, sizeof(bcs)); saved = bcs;
		sMockRegisters[kIntelN100BcsOffsets[invalid] / 4] = UINT32_MAX;
		assert(RunBcsInventory(&report, &bcs) == B_ERROR);
		assert(report.error == IntelN100Forcewake::InspectUncertain && report.ownership == IntelN100Forcewake::Unknown);
		assert(sRequestWrites == 1 && memcmp(&bcs, &saved, sizeof(bcs)) == 0);
		assert(sObservationPhase == ObservationUnknown); cases++;
	}
	Reset(); sEngineGate = true; memset(&bcs, 0xa5, sizeof(bcs)); saved = bcs;
	std::thread sampling([&] { result = RunBcsInventory(&report, &bcs); });
	{
		std::unique_lock<std::mutex> lock(sAckLock);
		assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEngineEntered; }));
	}
	assert(EndObservation(sObservationLease) == B_BAD_VALUE);
	ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy);
	{
		std::lock_guard<std::mutex> lock(sAckLock);
		sEngineProceed = true; sAckChanged.notify_all();
	}
	sampling.join();
	assert(result == B_ERROR && report.ownership == IntelN100Forcewake::Unknown);
	assert(sRequestWrites == 2 && memcmp(&bcs, &saved, sizeof(bcs)) == 0);
	assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	Reset(); sFailFinalOwner = true; memset(&bcs, 0xa5, sizeof(bcs)); saved = bcs;
	assert(RunBcsInventory(&report, &bcs) == B_ERROR);
	assert(report.stage == IntelN100Forcewake::OwnerValidation && report.ownership == IntelN100Forcewake::Unknown);
	assert(memcmp(&bcs, &saved, sizeof(bcs)) == 0 && sRequestWrites == 2); cases++;
	for (unsigned fault = 0; fault < 3; fault++) {
		Reset(); sEngineGate = true; memset(&bcs, 0xa5, sizeof(bcs)); saved = bcs;
		std::thread guarded([&] { result = RunBcsInventory(&report, &bcs); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEngineEntered; }));
		}
		if (fault == 0) {
			std::lock_guard<std::mutex> lock(sLock);
			Owner::Operation operation;
			assert(sState.BeginOperation(sDisplay, Owner::Free, operation) == Owner::Success);
			assert(sState.EndOperation(operation, true) == Owner::Success);
		}
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			if (fault == 1) sClock = 0;
			if (fault == 2) sClock += 50000;
			sEngineProceed = true; sAckChanged.notify_all();
		}
		guarded.join();
		assert(result == B_ERROR && report.ownership == IntelN100Forcewake::Unknown);
		assert(sRequestWrites == (fault == 0 ? 2u : 1u));
		assert(memcmp(&bcs, &saved, sizeof(bcs)) == 0);
		ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	// v5 fixed media fuse leaves the v4 BCS ABI and allowlist intact.
	Reset(); uint32 fuse = 0xfeedface;
	assert(RunMediaFuse(NULL, &fuse) == B_BAD_VALUE);
	assert(RunMediaFuse(&report, NULL) == B_BAD_VALUE);
	assert(RunGtTransaction(&report, &bcs, &fuse) == B_BAD_VALUE);
	assert(sRequestWrites == 0 && sReads == 0 && sBarriers == 0); cases++;
	assert(RunMediaFuse(&report, &fuse) == B_OK && fuse == 0x00010004);
	assert(report.error == 0 && report.cleanup == 0
		&& report.ownership == IntelN100Forcewake::Released
		&& report.stage == IntelN100Forcewake::Complete && sRequestWrites == 2);
	assert(sObservationPhase == ObservationIdle && !sObservationBlocked); cases++;
	for (unsigned fault = 0; fault < 5; fault++) {
		Reset(); fuse = 0xfeedface;
		if (fault == 0) sMockRegisters[0x9140 / 4] = UINT32_MAX;
		if (fault == 1) sNoSet = true;
		if (fault == 2) sNoClear = true;
		if (fault == 3) sFailFinalOwner = true;
		if (fault == 4) sBadPostRead = true;
		assert(RunMediaFuse(&report, &fuse) == B_ERROR && fuse == 0xfeedface);
		assert(report.ownership == IntelN100Forcewake::Unknown
			&& sObservationPhase == ObservationUnknown);
		unsigned writes = sRequestWrites, reads = sReads, barriers = sBarriers;
		IntelN100Forcewake::Report failure = report;
		assert(RunMediaFuse(&report, &fuse) == B_BUSY && fuse == 0xfeedface);
		assert(memcmp(&report, &failure, sizeof(report)) == 0);
		assert(sRequestWrites == writes && sReads == reads && sBarriers == barriers);
		ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	for (unsigned fault = 0; fault < 7; fault++) {
		Reset(); sEngineGate = true; fuse = 0xfeedface;
		std::thread samplingFuse([&] { result = RunMediaFuse(&report, &fuse); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sEngineEntered; }));
		}
		assert(EndObservation(sObservationLease) == B_BAD_VALUE);
		IntelN100Forcewake::Report ignored = {}; uint32 untouchedFuse = 0xabcdef;
		assert(RunMediaFuse(&ignored, &untouchedFuse) == B_BUSY && untouchedFuse == 0xabcdef);
		assert(RunBcsInventory(&ignored, &bcs) == B_BUSY);
		if (fault == 0) ReleaseDisplay();
		if (fault == 1) {
			std::lock_guard<std::mutex> lock(sConfigLock); sPMControl = 3;
		}
		if (fault == 2) {
			std::lock_guard<std::mutex> lock(sLock);
			Owner::Operation operation;
			assert(sState.BeginOperation(sDisplay, Owner::Free, operation) == Owner::Success);
			assert(sState.EndOperation(operation, true) == Owner::Success);
		}
		if (fault == 6) {
			std::lock_guard<std::mutex> lock(sLock); sRegistersUncached = false;
		}
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			if (fault == 3) sClock = 0;
			if (fault == 4) sClock += 50000;
			if (fault == 5) sMockRegisters[kIntelN100GtAckOffset / 4] = 0;
			sEngineProceed = true; sAckChanged.notify_all();
		}
		samplingFuse.join();
		assert(result == B_ERROR && fuse == 0xfeedface);
		assert(report.ownership == IntelN100Forcewake::Unknown);
		assert(sRequestWrites == (fault == 0 || fault == 2 ? 2u : 1u));
		if (fault != 0) ReleaseDisplay();
		assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	// Exercise actual adapter rejection before dereference: fixed endpoint
	// separation, alignment, BAR extent, address wrap and deadline/clock bounds.
	for (unsigned fault = 0; fault < 9; fault++) {
		Reset(); fuse = 0xfeedface;
		HandshakeContext context = {sDevice, {}, 1, sResources.registers,
			NULL, &fuse, false, 2000, 1000};
		uint32 offset = 0x9140, value = 0xabcdef;
		if (fault == 0) offset = 0xa188;
		if (fault == 1) offset = 0x22030;
		if (fault == 2) offset++;
		if (fault == 3) context.device.size = 0x9143;
		if (fault == 4) context.registers = UINTPTR_MAX - 0x9140;
		if (fault == 5) context.sampleDeadline = 1000;
		if (fault == 6) context.sampleLastTime = 1001;
		if (fault == 7) context.mediaFuse = NULL;
		if (fault == 8) { context.mediaFuse = NULL; context.inventory = &bcs; }
		assert(!ReadInventoryRegister(&context, offset, value));
		assert(context.sampleUncertain && value == 0xabcdef && sBarriers == 0); cases++;
	}
	// v6 owns one pin across fresh GT/fuse bootstrap and all-domain handshake.
	Reset(); sTestingGroup = true; sMockRegisters[0x9140/4] = 0xe00fe;
	IntelN100ForcewakeGroup::Report groupReport = {}; fuse = 0xfeedface;
	assert(RunGroupHandshake(NULL, &groupReport, &fuse) == B_BAD_VALUE);
	assert(RunGroupHandshake(&report, NULL, &fuse) == B_BAD_VALUE);
	assert(RunGroupHandshake(&report, &groupReport, NULL) == B_BAD_VALUE && sRequestWrites == 0);
	assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_OK);
	assert(fuse == 0xe00fe && groupReport.count == 4 && groupReport.workCalled == 0);
	assert(groupReport.error == 0 && groupReport.stage == IntelN100ForcewakeGroup::Complete);
	assert(sGroupWrites == 8 && sRequestWrites == 10 && sObservationPhase == ObservationIdle);
	for (unsigned i = 0; i < groupReport.count; i++)
		assert(groupReport.domains[i].ownership == IntelN100ForcewakeGroup::Released);
	unsigned groupReads = sReads, groupBarriers = sBarriers;
	auto groupSaved = groupReport;
	assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_BUSY);
	assert(memcmp(&groupSaved, &groupReport, sizeof(groupReport)) == 0
		&& sReads == groupReads && sBarriers == groupBarriers && sRequestWrites == 10); cases++;
	// A legacy transaction must not overwrite the completed v6 bootstrap.
	auto bootstrapSaved = report;
	sTestingGroup = false; sRequestWrites = 0; sClock += 200;
	assert(RunHandshake(&report) == B_OK);
	assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_BUSY);
	assert(memcmp(&bootstrapSaved, &report, sizeof(report)) == 0);
	assert(memcmp(&groupSaved, &groupReport, sizeof(groupReport)) == 0); cases++;
	for (unsigned fault = 0; fault < 8; fault++) {
		Reset(); sTestingGroup = true; sMockRegisters[0x9140/4] = 0xe00fe; fuse = 0xfeedface;
		if (fault == 0) sMockRegisters[0x9140/4] = UINT32_MAX;
		if (fault == 1) sMockRegisters[0xd84/4] = 1;
		if (fault == 2) sMockRegisters[0xd50/4] = 0x8000;
		if (fault == 3) sGroupNoSet = 1;
		if (fault == 4) sGroupNoClear = 4;
		if (fault == 5) sGroupBadRead = true;
		if (fault == 6) sGroupLoseAck = true;
		if (fault == 7) sFailGroupFinalOwner = true;
		assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_ERROR);
		if (fault == 7) {
			assert(sGroup.LastReport().stage == IntelN100ForcewakeGroup::Complete);
			assert(groupReport.error == IntelN100ForcewakeGroup::Unsafe && sGroupWrites == 8);
		}
		assert(fuse == 0xfeedface && sObservationPhase == ObservationUnknown && sObservationBlocked);
		unsigned writes = sRequestWrites, reads = sReads;
		assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_BUSY);
		assert(RunMediaFuse(&report, &fuse) == B_BUSY);
		assert(writes == sRequestWrites && reads == sReads);
		ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	for (unsigned fault = 0; fault < 4; fault++) {
		Reset(); sTestingGroup = sGroupGate = true; sMockRegisters[0x9140/4] = 0xe00fe; fuse = 0xfeedface;
		std::thread groupThread([&] { result = RunGroupHandshake(&report, &groupReport, &fuse); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sGroupEntered; }));
		}
		IntelN100ForcewakeGroup::Report ignoredGroup = {}; IntelN100Forcewake::Report ignoredBootstrap = {};
		uint32 ignoredFuse = 0xdeadbeef;
		assert(RunGroupHandshake(&ignoredBootstrap, &ignoredGroup, &ignoredFuse) == B_BUSY);
		assert(ignoredFuse == 0xdeadbeef && EndObservation(sObservationLease) == B_BAD_VALUE);
		if (fault == 0) ReleaseDisplay();
		if (fault == 1) { std::lock_guard<std::mutex> lock(sConfigLock); sPMControl = 3; }
		if (fault == 2) { std::lock_guard<std::mutex> lock(sLock); sRegistersUncached = false; }
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			if (fault == 3) sClock += 1000000;
			sGroupProceed = true; sAckChanged.notify_all();
		}
		groupThread.join();
		assert(result == B_ERROR && fuse == 0xfeedface && sGroupWrites == 1);
		assert(sObservationPhase == ObservationUnknown);
		if (fault != 0) ReleaseDisplay();
		assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	// Actual adapter: reject a fused-off domain, wrong access direction,
	// request masks other than the owned bit and invalid BAR extent/wrap.
	for (unsigned fault = 0; fault < 7; fault++) {
		Reset(); uint32 value = 0xabcdef;
		HandshakeContext handshake = {sDevice, {}, 1, sResources.registers, NULL, &fuse, false, 2000, 1000};
		GroupContext context = {}; context.handshake = &handshake;
		assert(intel_n100_reset_domains(0x46d18086, 0, 0xe00fe, context.plan));
		if (fault == 0) assert(!GroupRead(&context, 0xd58, value));
		if (fault == 1) assert(!GroupWrite(&context, 0xa548, 0x10001));
		if (fault == 2) assert(!GroupWrite(&context, 0xa188, 0x80008000));
		if (fault == 3) assert(!GroupRead(&context, 0xa188, value));
		if (fault == 4) assert(!GroupWrite(&context, 0xd84, 0x10001));
		if (fault == 5) { handshake.device.size = 0xd85; assert(!GroupRead(&context, 0xd84, value)); }
		if (fault == 6) { handshake.registers = UINTPTR_MAX - 0xd84; assert(!GroupRead(&context, 0xd84, value)); }
		assert(sRequestWrites == 0 && sBarriers == 0 && value == 0xabcdef); cases++;
	}
#ifdef INTEL_N100_BCS_RESET_H
	// v7 composes the actual retained owner with the group and reset helpers.
	using ResetHelper = IntelN100BcsReset;
	IntelN100BcsReset::Report resetReport = {};
	for (unsigned fault = 0; fault < 10; fault++) {
		Reset(); sTestingGroup = sTestingReset = true;
		sMockRegisters[0x9140/4] = 0xe00fe; fuse = 0xfeedface;
		sMockRegisters[ResetHelper::kControl/4] = 0;
		sMockRegisters[ResetHelper::kMode/4] = 0x200;
		sMockRegisters[ResetHelper::kPrefetch/4] = 0;
		sMockRegisters[ResetHelper::kResetControl/4] = 0;
		sMockRegisters[ResetHelper::kDomain/4] = 0;
		sMockRegisters[ResetHelper::kPending/4] = 0x10;
		if (fault == 1) sMockRegisters[ResetHelper::kControl/4] = 1;
		if (fault == 2) sMockRegisters[ResetHelper::kPrefetch/4] = UINT32_MAX;
		if (fault == 3) sResetNoReady = true;
		if (fault == 4) sResetNoDomain = true;
		if (fault == 5) sResetLoseAck = true;
		if (fault == 6) sFailGroupFinalOwner = true;
		if (fault == 7) sMockRegisters[0xd84/4] = 1;
		if (fault == 8 || fault == 9) {
			sResetLoseAck = true;
			sResetFaultAck = fault == 8 ? 0x8001 : UINT32_MAX;
		}
		assert(RunBcsReset(&report, &groupReport, &resetReport, &fuse) == (fault ? B_ERROR : B_OK));
#ifdef INTEL_N100_RESET_SAFETY_H
		IntelN100ResetSafety safety = {};
		unsigned readsBeforeSafety = sReads, writesBeforeSafety = sRequestWrites, barriersBeforeSafety = sBarriers;
		assert(ReadResetSafety(&safety) == B_OK);
		assert(sReads == readsBeforeSafety && sRequestWrites == writesBeforeSafety && sBarriers == barriersBeforeSafety);
		if (fault == 5 || fault == 8 || fault == 9) {
			assert(safety.reason == (fault == 9 ? IntelN100ResetSafety::AckRead : IntelN100ResetSafety::AckState)
				&& safety.domain == 1);
			assert(safety.phase == 2 && safety.ack[1] == sResetFaultAck);
		} else assert(safety.reason == IntelN100ResetSafety::None);
		assert(fault == 7 ? safety.checks == 0 : safety.checks > 0);
		cases++;
#endif
		if (fault == 0) {
			assert(resetReport.error == 0 && resetReport.stage == ResetHelper::ResetObserved);
			assert(resetReport.resetRounds == 2 && sResetWrites == 6 && sRequestWrites == 16);
			assert(groupReport.error == 0 && groupReport.workCalled == 1 && sGroupWrites == 8);
			assert(fuse == 0xe00fe && sObservationPhase == ObservationIdle);
		} else {
			assert(fuse == 0xfeedface && sObservationPhase == ObservationUnknown);
			assert(sGroupWrites == (fault == 6 ? 8u : fault == 7 ? 0u : 4u));
			if (fault == 1 || fault == 2 || fault == 7) assert(sResetWrites == 0);
			if (fault == 3) assert(sResetWrites == 3);
			if (fault == 4) assert(sResetWrites == 4);
			if (fault == 5 || fault == 8 || fault == 9)
				assert(sResetWrites == 1 && resetReport.error == ResetHelper::Unsafe);
			ReleaseDisplay(); assert(sState.BeginCleanup(true, true) == Owner::Busy);
		}
		auto savedReset = resetReport; auto savedGroup = groupReport; auto savedBootstrap = report;
		unsigned writes = sRequestWrites, reads = sReads;
		assert(RunBcsReset(&report, &groupReport, &resetReport, &fuse) == B_BUSY);
		assert(memcmp(&savedReset, &resetReport, sizeof(resetReport)) == 0);
		assert(memcmp(&savedGroup, &groupReport, sizeof(groupReport)) == 0);
		assert(memcmp(&savedBootstrap, &report, sizeof(report)) == 0);
		assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_BUSY);
		assert(sRequestWrites == writes && sReads == reads); cases++;
		#ifdef INTEL_N100_RESET_SAFETY_H
		IntelN100ResetSafety repeatedSafety = {};
		assert(ReadResetSafety(&repeatedSafety) == B_OK);
		assert(memcmp(&safety, &repeatedSafety, sizeof(safety)) == 0); cases++;
		#endif
	}
	for (unsigned fault = 0; fault < 4; fault++) {
		Reset(); sTestingGroup = sTestingReset = sResetGate = true;
		sMockRegisters[0x9140/4] = 0xe00fe; fuse = 0xfeedface;
		sMockRegisters[ResetHelper::kControl/4] = 0;
		sMockRegisters[ResetHelper::kMode/4] = 0x200;
		sMockRegisters[ResetHelper::kPrefetch/4] = 0;
		sMockRegisters[ResetHelper::kResetControl/4] = 0;
		sMockRegisters[ResetHelper::kDomain/4] = 0;
		sMockRegisters[ResetHelper::kPending/4] = 0x10;
		std::thread resetThread([&] { result = RunBcsReset(&report, &groupReport, &resetReport, &fuse); });
		{
			std::unique_lock<std::mutex> lock(sAckLock);
			assert(sAckChanged.wait_for(lock, std::chrono::seconds(5), [] { return sResetEntered; }));
		}
		IntelN100Forcewake::Report ignoredBootstrap = {};
		IntelN100ForcewakeGroup::Report ignoredGroup = {};
		IntelN100BcsReset::Report ignoredReset = {}; uint32 ignoredFuse = 0xabcdef;
		assert(RunBcsReset(&ignoredBootstrap, &ignoredGroup, &ignoredReset, &ignoredFuse) == B_BUSY);
#ifdef INTEL_N100_RESET_SAFETY_H
		IntelN100ResetSafety pendingSafety = {}; pendingSafety.reason = 123;
		assert(ReadResetSafety(&pendingSafety) == B_BUSY && pendingSafety.reason == 123);
#endif
		assert(RunGroupHandshake(&ignoredBootstrap, &ignoredGroup, &ignoredFuse) == B_BUSY);
		assert(ignoredFuse == 0xabcdef && EndObservation(sObservationLease) == B_BAD_VALUE);
		if (fault == 0) ReleaseDisplay();
		if (fault == 1) { std::lock_guard<std::mutex> lock(sConfigLock); sPMControl = 3; }
		if (fault == 2) { std::lock_guard<std::mutex> lock(sLock); sRegistersUncached = false; }
		{
			std::lock_guard<std::mutex> lock(sAckLock);
			if (fault == 3) sClock += 1000000;
			sResetProceed = true; sAckChanged.notify_all();
		}
		resetThread.join();
		assert(result == B_ERROR && fuse == 0xfeedface && sResetWrites == 1 && sGroupWrites == 4);
#ifdef INTEL_N100_RESET_SAFETY_H
		IntelN100ResetSafety failureSafety = {};
		assert(ReadResetSafety(&failureSafety) == B_OK);
		if (fault == 0) assert(failureSafety.reason == IntelN100ResetSafety::Owner);
		if (fault == 1) assert(failureSafety.reason == IntelN100ResetSafety::PciCapture);
		if (fault == 2) assert(failureSafety.reason == IntelN100ResetSafety::Mapping);
		// The helper's own clock can reject before the adapter is called.
		if (fault == 3) assert(resetReport.error == ResetHelper::Deadline);
		cases++;
#endif
		assert(sObservationPhase == ObservationUnknown);
		if (fault != 0) ReleaseDisplay();
		assert(sState.BeginCleanup(true, true) == Owner::Busy); cases++;
	}
	Reset(); sTestingGroup = true; sMockRegisters[0x9140/4] = 0xe00fe;
	assert(RunGroupHandshake(&report, &groupReport, &fuse) == B_OK);
	assert(RunBcsReset(&report, &groupReport, &resetReport, &fuse) == B_BUSY);
	assert(!sBcsReset.WasUsed() && sResetWrites == 0 && sRequestWrites == 10); cases++;
	Reset();
	assert(RunBcsReset(NULL, &groupReport, &resetReport, &fuse) == B_BAD_VALUE);
#ifdef INTEL_N100_RESET_SAFETY_H
	IntelN100ResetSafety beforeReset = {}; beforeReset.reason = 123;
	assert(ReadResetSafety(NULL) == B_BAD_VALUE);
	assert(ReadResetSafety(&beforeReset) == B_BUSY && beforeReset.reason == 123); cases++;
#endif
	assert(RunBcsReset(&report, NULL, &resetReport, &fuse) == B_BAD_VALUE);
	assert(RunBcsReset(&report, &groupReport, NULL, &fuse) == B_BAD_VALUE);
	assert(RunBcsReset(&report, &groupReport, &resetReport, NULL) == B_BAD_VALUE);
	assert(sReads == 0 && sRequestWrites == 0); cases++;
	for (unsigned fault = 0; fault < 7; fault++) {
		Reset(); uint32 value = 0xabcdef;
		HandshakeContext handshake = {sDevice, {}, 1, sResources.registers, NULL, &fuse, false, 2000, 1000};
		GroupContext context = {}; context.handshake = &handshake;
		if (fault == 0) assert(!ResetRead(&context, 0x22030, value));
		if (fault == 1) assert(!ResetWrite(&context, 0x941c, 8));
		if (fault == 2) assert(!ResetWrite(&context, 0x800c, 0));
		if (fault == 3) assert(!ResetWrite(&context, 0x2209c, 0x01000000));
		if (fault == 4) assert(!ResetWrite(&context, 0x220d0, 0x80008000));
		if (fault == 5) { handshake.device.size = 0x941f; assert(!ResetRead(&context, 0x941c, value)); }
		if (fault == 6) { handshake.registers = UINTPTR_MAX - 0x941c; assert(!ResetRead(&context, 0x941c, value)); }
		assert(sRequestWrites == 0 && sBarriers == 0 && value == 0xabcdef); cases++;
	}
#endif
	printf("%u actual owner observation cases passed; mocked PCI/kernel only.\n", cases);
}
