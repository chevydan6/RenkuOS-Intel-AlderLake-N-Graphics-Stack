/* SPDX-License-Identifier: MIT */
#include "AlderLakeHdmiSequence.h"
#include <stdio.h>

struct Mock {
	uint32 power, buffer, status, hip, pll, portClock, clockGates, clock, function, phy[16384];
	unsigned writes, failAt, pauses;
};
static status_t Read(void* p, uint32 offset, uint32* value) {
	Mock& m = *(Mock*)p;
	if (offset == 0x45444) *value = m.power;
	else if (offset == 0x64400) *value = m.buffer;
	else if (offset == 0x161504) *value = m.status;
	else if (offset == 0x46040) *value = m.pll;
	else if (offset == 0x46110) *value = m.portClock;
	else if (offset == 0x164280) *value = m.clockGates;
	else if (offset == 0x46144) *value = m.clock;
	else if (offset == 0x61400) *value = m.function;
	else if (offset >= 0x169000 && offset < 0x16a000) {
		unsigned bank = (m.hip >> 8) & 15;
		*value = m.phy[(bank * 0x1000 + offset - 0x169000) / 4];
	} else return B_BAD_VALUE;
	return B_OK;
}
static status_t Write(void* p, uint32 offset, uint32 value) {
	Mock& m = *(Mock*)p;
	m.writes++;
	if (m.failAt && m.writes == m.failAt) return B_ERROR;
	if (offset == 0x45444) {
		m.power = value;
		if (value & (1U << 9)) m.power |= 1U << 8; else m.power &= ~(1U << 8);
	} else if (offset == 0x64400) {
		m.buffer = value & ~(1U << 7);
		if (!(value & (1U << 31))) m.buffer |= 1U << 7;
	} else if (offset == 0x1010a0) m.hip = value;
	else if (offset == 0x46040) {
		m.pll = value & ~((1U << 30) | (1U << 26));
		if (value & (1U << 27)) m.pll |= 1U << 26;
		if (value & (1U << 31)) m.pll |= 1U << 30;
	} else if (offset == 0x46110) m.portClock = value;
	else if (offset == 0x164280) m.clockGates = value;
	else if (offset == 0x46144) m.clock = value;
	else if (offset == 0x61400) m.function = value;
	else if (offset >= 0x169000 && offset < 0x16a000) {
		unsigned bank = (m.hip >> 8) & 15;
		m.phy[(bank * 0x1000 + offset - 0x169000) / 4] = value;
	} else return B_BAD_VALUE;
	return B_OK;
}
static void Pause(void* p) { ((Mock*)p)->pauses++; }
static status_t WellRead(void* p, uint32* value) { return Read(p, 0x45444, value); }
static status_t WellWrite(void* p, uint32 value) { return Write(p, 0x45444, value); }
static AlderLakeTypeCIO IO(Mock& m) { AlderLakeTypeCIO io = {&m, Read, Write, Pause}; return io; }
static TypeCPowerWellOps WellOps(Mock& m) { TypeCPowerWellOps ops = {&m, WellRead, WellWrite, Pause}; return ops; }
static void Reset(Mock& m) {
	m = {}; m.buffer = 1U << 7; m.status = 1U << 2; m.function = 0x30000;
	m.portClock = 0x123; m.clockGates = 0x40002000;
	for (unsigned i = 0; i < 16384; i++) m.phy[i] = 0x12345678;
	m.phy[0x236c / 4] |= 1U << 15;
}
static bool Restored(const Mock& m) {
	if (m.power || m.buffer != (1U << 7) || m.pll || m.clock || m.function != 0x30000
		|| m.portClock != 0x123 || m.clockGates != 0x40002000) return false;
	for (unsigned i = 0; i < 16384; i++) {
		uint32 expected = 0x12345678;
		if (i == 0x236c / 4) expected |= 1U << 15;
		if (m.phy[i] != expected) return false;
	}
	return true;
}
static status_t Begin(AlderLakeHdmiSequence& s) {
	return s.Begin(148500, 19200, 9, false, true, true, false);
}
int main() {
	unsigned errors = 0, beginWrites = 0; Mock m; uint32 leases = 0; Reset(m);
	{
		TypeCPowerWell well(WellOps(m), 2, 3); TypeCPortLease lease(&leases, 2);
		AlderLakeHdmiSequence s(IO(m), well, lease, 2, 1, 3);
		if (Begin(s) != B_OK || !(m.buffer & 0x80000000U) || m.clock != 0x70000000U
			|| m.function != 0xa8030000U || m.portClock != 0x80000123U
			|| (m.clockGates & 0x2000) != 0 || Begin(s) != B_BUSY || leases != 2) errors++;
		beginWrites = m.writes;
		if (s.End() != B_OK || !Restored(m) || s.End() != B_OK || leases != 0) errors++;
	}
	for (unsigned failure = 1; failure <= beginWrites; failure++) {
		Reset(m); leases = 0; m.failAt = failure;
		TypeCPowerWell well(WellOps(m), 2, 3); TypeCPortLease lease(&leases, 2);
		AlderLakeHdmiSequence s(IO(m), well, lease, 2, 1, 3);
		if (Begin(s) == B_OK || !Restored(m) || well.IsHeld() || leases != 0) {
			fprintf(stderr, "failed rollback at write %u\n", failure); errors++;
		}
	}
	Reset(m);
	{
		TypeCPowerWell well(WellOps(m), 2, 3); TypeCPortLease lease(&leases, 2);
		AlderLakeHdmiSequence s(IO(m), well, lease, 2, 1, 3);
		if (Begin(s) != B_OK) errors++;
		m.failAt = m.writes + 1;
		if (s.End() == B_OK || Begin(s) != B_BUSY || leases != 2) errors++;
		m.failAt = 0;
		if (s.End() != B_OK || !Restored(m) || leases != 0) errors++;
	}
	Reset(m);
	{
		TypeCPowerWell well(WellOps(m), 2, 3); TypeCPortLease lease(&leases, 2);
		AlderLakeHdmiSequence s(IO(m), well, lease, 2, 1, 3);
		if (s.Begin(148500, 19200, 10, false, true, true, false) != B_BAD_VALUE
			|| m.writes != 0) errors++;
	}
	Reset(m);
	{
		TypeCPowerWell well(WellOps(m), 2, 3); TypeCPortLease lease(&leases, 2);
		TypeCPortLease competitor(&leases, 2); TypeCPortLease other(&leases, 1);
		AlderLakeHdmiSequence s(IO(m), well, lease, 2, 1, 3);
		if (competitor.Acquire() != B_OK || Begin(s) != B_BUSY || m.writes != 0
			|| other.Acquire() != B_OK || other.Release() != B_OK
			|| competitor.Release() != B_OK || leases != 0) errors++;
	}
	if (errors) { fprintf(stderr, "%u HDMI sequence cases failed\n", errors); return 1; }
	printf("HDMI sequence and %u single-write rollback cases passed; mock registers only\n", beginWrites);
	return 0;
}
