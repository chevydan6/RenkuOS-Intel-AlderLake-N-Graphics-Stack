// SPDX-License-Identifier: MIT
#include "IntelN100Forcewake.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

using Wake = IntelN100Forcewake;
struct Mock {
	uint64_t now = 1000;
	Wake* wake = NULL;
	unsigned reads = 0, writes = 0, powers = 0, lives = 0;
	int failRead = -1, onesRead = -1, failWrite = -1, failPower = -1, failLive = -1;
	int fallbackRead = -1, jumpRead = -1;
	int backwardsTime = -1;
	unsigned clocks = 0;
	unsigned setDelay = 0, clearDelay = 0, initialDelay = 0;
	bool neverSet = false, neverClear = false, frozen = false, backwards = false;
	bool closeAfterEnable = false, closed = false;
	bool mustRead = false;
	int delayPower = -1;
	int slowRead = -1;
	uint64_t readDelay = 0;
	std::vector<char> events;
	std::vector<uint32_t> words;
};
static uint64_t Now(void* context)
{
	Mock& m = *(Mock*)context;
	assert(!m.mustRead);
	if (m.backwards || int(m.clocks++) == m.backwardsTime) return 0;
	return m.now;
}
static bool Read(void* context, uint32_t& ack)
{
	Mock& m = *(Mock*)context;
	m.mustRead = false;
	m.events.push_back('R');
	unsigned read = m.reads++;
	if (int(read) == m.slowRead) m.now += m.readDelay;
	if (int(read) == m.jumpRead) m.now += 300000;
	if (int(read) == m.failRead) return false;
	if (int(read) == m.onesRead) { ack = UINT32_MAX; return true; }
	if (int(read) == m.fallbackRead) { ack = 0x8000; return true; }
	if (m.writes == 0) {
		ack = m.initialDelay > 0 ? 1 : 0;
		if (m.initialDelay) m.initialDelay--;
	}
	else if (m.writes == 1) {
		ack = !m.neverSet && m.setDelay == 0 ? 1 : 0;
		if (m.setDelay) m.setDelay--;
	} else {
		ack = m.neverClear || m.clearDelay > 0 ? 1 : 0;
		if (m.clearDelay) m.clearDelay--;
	}
	return true;
}
static bool Write(void* context, uint32_t word)
{
	Mock& m = *(Mock*)context;
	assert(!m.mustRead);
	m.events.push_back('W');
	assert(m.wake->LastReport().writes == m.writes + 1);
	assert(m.wake->LastReport().ownership == (m.writes == 0 ? Wake::EnableAttempted : Wake::ClearAttempted));
	assert(word == (m.writes == 0 ? 0x10001 : 0x10000));
	m.words.push_back(word);
	if (m.closeAfterEnable && m.writes == 0) m.closed = true;
	bool issued = int(m.writes++) != m.failWrite;
	m.mustRead = issued;
	return issued;
}
static bool Power(void* context)
{
	Mock& m = *(Mock*)context; m.events.push_back('P');
	assert(!m.mustRead);
	if (int(m.powers) == m.delayPower) m.now += 210000;
	return int(m.powers++) != m.failPower;
}
static bool Live(void* context)
{
	Mock& m = *(Mock*)context; m.events.push_back('L');
	assert(!m.mustRead);
	return int(m.lives++) != m.failLive && !m.closed;
}
static void Pause(void* context)
{
	Mock& m = *(Mock*)context; m.events.push_back('S');
	assert(!m.mustRead);
	if (!m.frozen) m.now += 100;
}
static Wake::IO Callbacks(Mock& m)
{
	return {&m, Now, Read, Write, Power, Live, Pause};
}
static unsigned sCases;
static void Check(Mock m, Wake::Error expected, Wake::Ownership ownership, unsigned writes)
{
	Wake wake; m.wake = &wake;
	assert(wake.Run(Callbacks(m)) == expected);
	const Wake::Report& r = wake.LastReport();
	assert(r.error == unsigned(expected) && r.ownership == unsigned(ownership));
	assert(r.writes == writes && m.words.size() == writes);
	assert(r.reads == m.reads && r.reads <= 1536);
	for (uint32_t word : m.words) assert(word == 0x10001 || word == 0x10000);
	if (expected != Wake::Ok) {
		Wake::Report saved = r;
		unsigned oldReads = m.reads, oldWrites = m.writes;
		size_t events = m.events.size();
		assert(wake.IsBlocked() && wake.Run(Callbacks(m)) == Wake::Blocked);
		assert(memcmp(&saved, &wake.LastReport(), sizeof(saved)) == 0);
		assert(m.reads == oldReads && m.writes == oldWrites);
		assert(m.events.size() == events);
	}
	sCases++;
}
int main()
{
	Mock m;
	Check(m, Wake::Ok, Wake::Released, 2);
	for (unsigned delay : {1u, 7u, 100u, 499u}) {
		m = Mock(); m.setDelay = delay; m.clearDelay = delay;
		Check(m, Wake::Ok, Wake::Released, 2);
	}
	m = Mock(); m.initialDelay = 1000; Check(m, Wake::PreTimeout, Wake::NoRequest, 0);
	m = Mock(); m.neverSet = true; Check(m, Wake::SetTimeout, Wake::Unknown, 2);
	m = Mock(); m.neverClear = true; Check(m, Wake::ClearTimeout, Wake::Unknown, 2);
	m = Mock(); m.frozen = true; m.neverSet = true; Check(m, Wake::SetTimeout, Wake::Unknown, 2);
	for (int read = 0; read < 3; read++) {
		m = Mock(); m.failRead = read;
		Check(m, Wake::ReadError, read ? Wake::Unknown : Wake::NoRequest, read == 0 ? 0 : read == 1 ? 1 : 2);
		m = Mock(); m.onesRead = read;
		Check(m, Wake::ReadError, read ? Wake::Unknown : Wake::NoRequest, read == 0 ? 0 : read == 1 ? 1 : 2);
		m = Mock(); m.fallbackRead = read;
		Check(m, Wake::FallbackChanged, read ? Wake::Unknown : Wake::NoRequest, read == 0 ? 0 : read == 1 ? 1 : 2);
		m = Mock(); m.jumpRead = read;
		Check(m, Wake::TotalTimeout, read ? Wake::Unknown : Wake::NoRequest, read == 0 ? 0 : read == 1 ? 1 : 2);
	}
	for (int write = 0; write < 2; write++) {
		m = Mock(); m.failWrite = write;
		Check(m, Wake::WriteError, Wake::Unknown, write + 1);
	}
	for (int power = 0; power < 4; power++) {
		m = Mock(); m.failPower = power;
		Check(m, Wake::PowerChanged, power < 2 ? Wake::NoRequest : Wake::Unknown, power < 2 ? 0 : power == 2 ? 1 : 2);
	}
	m = Mock(); m.failLive = 0; Check(m, Wake::Cancelled, Wake::NoRequest, 0);
	m = Mock(); m.failLive = 1; Check(m, Wake::Cancelled, Wake::NoRequest, 0);
	m = Mock(); m.failLive = 2; Check(m, Wake::Cancelled, Wake::Unknown, 2);
	m = Mock(); m.closeAfterEnable = true; Check(m, Wake::Cancelled, Wake::Unknown, 2);
	m = Mock(); m.closeAfterEnable = true; m.neverSet = true; Check(m, Wake::Cancelled, Wake::Unknown, 2);
	m = Mock(); m.closeAfterEnable = true; m.onesRead = 1; Check(m, Wake::ReadError, Wake::Unknown, 1);
	m = Mock(); m.closeAfterEnable = true; m.fallbackRead = 1; Check(m, Wake::FallbackChanged, Wake::Unknown, 1);
	for (int power = 0; power < 3; power++) {
		m = Mock(); m.delayPower = power;
		Check(m, Wake::TotalTimeout, power < 2 ? Wake::NoRequest : Wake::Unknown, power < 2 ? 0 : 1);
	}
	for (int read = 0; read < 3; read++) {
		for (uint64_t delay : {49000u, 50000u, 51000u, 60000u}) {
			m = Mock(); m.slowRead = read; m.readDelay = delay;
			Wake::Error error = delay < 50000 ? Wake::Ok
				: read == 0 ? Wake::PreTimeout : read == 1 ? Wake::SetTimeout : Wake::ClearTimeout;
			Check(m, error, error == Wake::Ok ? Wake::Released : read == 0 ? Wake::NoRequest : Wake::Unknown,
				error == Wake::PreTimeout ? 0 : 2);
		}
	}
	m = Mock(); m.slowRead = 1; m.readDelay = 49000; m.setDelay = 20;
	Check(m, Wake::SetTimeout, Wake::Unknown, 2);
	m = Mock(); m.slowRead = 2; m.readDelay = 49000; m.clearDelay = 20;
	Check(m, Wake::ClearTimeout, Wake::Unknown, 2);
	m = Mock(); m.now = UINT64_MAX - 249999; Check(m, Wake::ClockError, Wake::NoRequest, 0);
	Wake success; m = Mock(); m.wake = &success;
	assert(success.Run(Callbacks(m)) == Wake::Ok);
	const char order[] = "PLRLPWRPWRPL";
	assert(m.events.size() == sizeof(order) - 1);
	assert(memcmp(m.events.data(), order, sizeof(order) - 1) == 0); sCases++;
	unsigned clocks = m.clocks;
	// Every clock observation must reject a backwards value, including cleanup.
	for (unsigned time = 1; time < clocks; time++) {
		Wake wake; m = Mock(); m.wake = &wake; m.backwardsTime = time;
		assert(wake.Run(Callbacks(m)) == Wake::ClockError);
		assert(wake.LastReport().ownership == (m.writes ? Wake::Unknown : Wake::NoRequest)); sCases++;
	}
	// An incomplete callback table performs no I/O.
	for (unsigned callback = 0; callback < 6; callback++) {
		Wake wake; m = Mock(); m.wake = &wake; Wake::IO io = Callbacks(m);
		switch (callback) {
			case 0: io.now = NULL; break; case 1: io.readAck = NULL; break;
			case 2: io.writeRequest = NULL; break; case 3: io.pciStable = NULL; break;
			case 4: io.live = NULL; break; case 5: io.pause = NULL; break;
		}
		assert(wake.Run(io) == Wake::InvalidCallbacks && m.reads == 0 && m.writes == 0); sCases++;
	}
	printf("%u forcewake state-machine cases passed; mocked I/O only.\n", sCases);
}
