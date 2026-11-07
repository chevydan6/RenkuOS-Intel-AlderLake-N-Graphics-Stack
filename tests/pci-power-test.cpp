// SPDX-License-Identifier: MIT
#include "IntelN100PciPower.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef IntelN100PciPower Power;
static const uint64_t kBase = 0x6000000000ULL;
static const uint64_t kSize = 0x1000000;
struct Config {
	uint8_t bytes[256];
	unsigned calls;
	int failAt;
	int mutateAt;
	uint16_t mutateOffset;
	uint8_t mutateValue;
};

static void Put(Config& config, unsigned offset, unsigned size, uint32_t value)
{
	assert(offset + size <= 256);
	for (unsigned i = 0; i < size; i++)
		config.bytes[offset + i] = value >> (i * 8);
}

static Config Good()
{
	Config config = {};
	config.failAt = config.mutateAt = -1;
	Put(config, 0, 4, 0x46d18086);
	Put(config, 8, 4, 0x03000000);
	Put(config, 4, 2, 2); // Memory decoding; bus mastering deliberately clear.
	Put(config, 6, 2, 0x10);
	Put(config, 0x10, 4, uint32_t(kBase) | 4);
	Put(config, 0x14, 4, kBase >> 32);
	Put(config, 0x34, 1, 0x40);
	Put(config, 0x40, 2, 0x4805); // MSI, then PM.
	Put(config, 0x48, 2, 1);
	Put(config, 0x4a, 2, 3);
	return config;
}

static bool Read(void* context, uint16_t offset, uint8_t size, uint32_t& value)
{
	Config& config = *(Config*)context;
	assert(size == 1 || size == 2 || size == 4);
	assert(offset + size <= 256 && (offset & (size - 1)) == 0);
	unsigned call = config.calls++;
	assert(call < 256); // Test watchdog independent of the walk implementation.
	if (int(call) == config.failAt)
		return false;
	if (int(call) == config.mutateAt)
		config.bytes[config.mutateOffset] = config.mutateValue;
	value = 0;
	for (unsigned i = 0; i < size; i++)
		value |= uint32_t(config.bytes[offset + i]) << (i * 8);
	return true;
}

static unsigned sCases;
static void Check(Config config, Power::Result expected,
	uint64_t base = kBase, uint64_t size = kSize)
{
	IntelN100PciSnapshot result, saved;
	memset(&result, 0xa5, sizeof(result));
	saved = result;
	assert(Power::Capture(Read, &config, base, size, result) == expected);
	if (expected != Power::Success)
		assert(memcmp(&result, &saved, sizeof(result)) == 0);
	else {
		assert(result.identity == 0x46d18086 && result.command == 2);
		assert(result.pmOffset == 0x48 && result.pmControl == 0);
	}
	sCases++;
}

int main()
{
	Check(Good(), Power::Success);
	Config config = Good();
	IntelN100PciSnapshot snapshot = {};
	assert(Power::Capture(Read, &config, kBase, kSize, snapshot) == Power::Success);
	unsigned successfulReads = config.calls;
	for (unsigned i = 0; i < successfulReads; i++) {
		config = Good(); config.failAt = i;
		Check(config, Power::ReadFailed);
	}
	for (unsigned state = 1; state <= 3; state++) {
		config = Good(); Put(config, 0x4c, 2, state);
		Check(config, Power::NotD0);
	}
	config = Good(); Put(config, 0, 4, 0xffffffff); Check(config, Power::ReadFailed);
	config = Good(); Put(config, 0, 4, 0x46d28086); Check(config, Power::UnsupportedDevice);
	config = Good(); Put(config, 8, 4, 0x03000001); Check(config, Power::UnsupportedDevice);
	config = Good(); Put(config, 0x0e, 1, 1); Check(config, Power::UnsupportedDevice);
	config = Good(); Put(config, 4, 2, 0); Check(config, Power::MemoryDisabled);
	config = Good(); Put(config, 6, 2, 0); Check(config, Power::MissingPower);
	config = Good(); Put(config, 0x34, 1, 0); Check(config, Power::MissingPower);
	config = Good(); Put(config, 0x48, 2, 5); Check(config, Power::MissingPower);
	config = Good(); Put(config, 0x48, 2, 0x4001); Check(config, Power::InvalidCapabilities);
	config = Good(); Put(config, 0x40, 2, 0x4801); Check(config, Power::InvalidCapabilities);
	for (unsigned pointer = 1; pointer < 256; pointer++) {
		if (pointer >= 0x40 && pointer <= 0xfc && pointer % 4 == 0)
			continue;
		config = Good(); Put(config, 0x34, 1, pointer);
		Check(config, pointer == 255 ? Power::ReadFailed : Power::InvalidCapabilities);
	}
	config = Good(); Put(config, 0x34, 1, 0xfc); Put(config, 0xfc, 2, 1);
	Check(config, Power::InvalidCapabilities);
	config = Good(); Put(config, 0x48, 2, 0x4c01); Put(config, 0x4c, 2, 4);
	Check(config, Power::InvalidCapabilities);
	config = Good(); Put(config, 0x34, 1, 0x4c); Put(config, 0x4c, 2, 0x4804);
	Check(config, Power::InvalidCapabilities);
	config = Good(); Put(config, 0x4c, 2, 0xffff); Check(config, Power::ReadFailed);
	for (unsigned bar = 0; bar < 8; bar++) {
		if (bar == 4) continue;
		config = Good(); Put(config, 0x10, 4, bar); Check(config, Power::InvalidBar);
	}
	Check(Good(), Power::InvalidBar, kBase + 4096);
	Check(Good(), Power::InvalidMapping, 0);
	Check(Good(), Power::InvalidMapping, kBase + 1);
	Check(Good(), Power::InvalidMapping, kBase, 0);
	Check(Good(), Power::InvalidMapping, kBase, kSize + 1);
	Check(Good(), Power::InvalidMapping, UINT64_MAX - 4095, 8192);
	// Config changes during and between complete walks must not publish output.
	for (unsigned call = 8; call < successfulReads - 7; call++) {
		config = Good(); config.mutateAt = call;
		config.mutateOffset = 0x14; config.mutateValue = 0x61;
		Check(config, Power::InvalidBar);
	}
	config = Good(); config.mutateAt = 12; config.mutateOffset = 4;
	config.mutateValue = 6; Check(config, Power::Unstable);
	// Longest possible acyclic walk is bounded and reaches the final valid PM.
	config = Good();
	for (unsigned offset = 0x40; offset <= 0xf4; offset += 4)
		Put(config, offset, 2, 5 | ((offset + 4) << 8));
	Put(config, 0xf8, 2, 1); Put(config, 0xfa, 2, 3); Put(config, 0xfc, 2, 0);
	assert(Power::Capture(Read, &config, kBase, kSize, snapshot) == Power::Success);
	sCases++;
	assert(snapshot.pmOffset == 0xf8);
	assert(Power::Capture(NULL, NULL, kBase, kSize, snapshot) == Power::ReadFailed);
	sCases++;
	printf("%u PCI power cases passed; mocked config reads only.\n", sCases);
}
