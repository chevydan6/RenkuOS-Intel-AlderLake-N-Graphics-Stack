// SPDX-License-Identifier: MIT
#ifndef INTEL_N100_PCI_POWER_H
#define INTEL_N100_PCI_POWER_H

#include <stddef.h>
#include <stdint.h>

// Read-only PCI admission for the retained N100 owner. This does not hold power
// against firmware, shutdown, or another PCI client, and authorizes no GT access.
// The callback binds the owner's BDF, returns raw config values and reports any
// transport error it can detect. PCI's raw API may only signal failure as ones.
struct IntelN100PciSnapshot {
	uint32_t identity;
	uint32_t classRevision;
	uint32_t barLow;
	uint32_t barHigh;
	uint16_t command;
	uint16_t status;
	uint16_t pmCapabilities;
	uint16_t pmControl;
	uint16_t capabilities[48];
	uint8_t headerType;
	uint8_t firstCapability;
	uint8_t pmOffset;
};

class IntelN100PciPower {
public:
	typedef bool (*Reader)(void*, uint16_t, uint8_t, uint32_t&);
	enum Result {
		Success, InvalidMapping, ReadFailed, UnsupportedDevice,
		MemoryDisabled, InvalidBar, InvalidCapabilities, MissingPower,
		NotD0, Unstable
	};

	static bool Same(const IntelN100PciSnapshot& a,
		const IntelN100PciSnapshot& b)
	{
		if (a.identity != b.identity || a.classRevision != b.classRevision
			|| a.barLow != b.barLow || a.barHigh != b.barHigh
			|| a.command != b.command || a.status != b.status
			|| a.headerType != b.headerType
			|| a.firstCapability != b.firstCapability || a.pmOffset != b.pmOffset
			|| a.pmCapabilities != b.pmCapabilities || a.pmControl != b.pmControl)
			return false;
		for (unsigned i = 0; i < 48; i++) {
			if (a.capabilities[i] != b.capabilities[i])
				return false;
		}
		return true;
	}

	// Two complete captures bracket the capability walk with identity, command,
	// BAR and PM-state observations. Equality is evidence at the sampled times,
	// never proof that power or config cannot change after return. Output is
	// committed only after all validation succeeds. No PCI writes are provided.
	// ownedSize is trusted owner metadata, not a BAR-size probe. Future GT
	// accessors must independently check each register against this extent.
	static Result Capture(Reader read, void* context, uint64_t ownedBase,
		uint64_t ownedSize, IntelN100PciSnapshot& output)
	{
		if (ownedBase == 0 || (ownedBase & 4095) != 0
			|| ownedSize < 4096 || (ownedSize & 4095) != 0
			|| ownedBase > UINT64_MAX - ownedSize)
			return InvalidMapping;
		IntelN100PciSnapshot first = {}, second = {};
		Result result = Once(read, context, ownedBase, first);
		if (result != Success)
			return result;
		result = Once(read, context, ownedBase, second);
		if (result != Success)
			return result;
		if (!Same(first, second))
			return Unstable;
		output = second;
		return Success;
	}

private:
	static bool Read(Reader reader, void* context, uint16_t offset,
		uint8_t size, uint32_t& value)
	{
		if (reader == NULL || !reader(context, offset, size, value))
			return false;
		const uint32_t maximum = size == 4 ? UINT32_MAX
			: size == 2 ? 0xffff : 0xff;
		return value < maximum; // Reject all ones and unrepresentable reads.
	}

	static Result Anchors(Reader read, void* context, uint64_t ownedBase,
		IntelN100PciSnapshot& sample)
	{
		uint32_t command = 0, status = 0, header = 0;
		if (!Read(read, context, 0, 4, sample.identity)
			|| !Read(read, context, 8, 4, sample.classRevision)
			|| !Read(read, context, 0x0e, 1, header)
			|| !Read(read, context, 4, 2, command)
			|| !Read(read, context, 6, 2, status)
			|| !Read(read, context, 0x10, 4, sample.barLow)
			|| !Read(read, context, 0x14, 4, sample.barHigh))
			return ReadFailed;
		sample.headerType = header;
		sample.command = command;
		sample.status = status;
		if (sample.identity != 0x46d18086 || sample.classRevision != 0x03000000
			|| (header & 0x7f) != 0)
			return UnsupportedDevice;
		if ((command & 2) == 0)
			return MemoryDisabled;
		uint64_t base = (uint64_t(sample.barHigh) << 32)
			| (sample.barLow & ~uint32_t(15));
		if ((sample.barLow & 7) != 4 || base != ownedBase)
			return InvalidBar;
		return Success;
	}

	static Result Once(Reader read, void* context, uint64_t ownedBase,
		IntelN100PciSnapshot& sample)
	{
		Result result = Anchors(read, context, ownedBase, sample);
		if (result != Success)
			return result;
		if ((sample.status & 0x10) == 0)
			return MissingPower;
		uint32_t pointer = 0;
		if (!Read(read, context, 0x34, 1, pointer))
			return ReadFailed;
		sample.firstCapability = pointer;
		uint64_t visited = 0;
		while (pointer != 0) {
			if (pointer < 0x40 || pointer > 0xfc || (pointer & 3) != 0)
				return InvalidCapabilities;
			unsigned index = (pointer - 0x40) / 4;
			if ((visited & (uint64_t(1) << index)) != 0)
				return InvalidCapabilities;
			visited |= uint64_t(1) << index;
			uint32_t capability = 0;
			if (!Read(read, context, pointer, 2, capability))
				return ReadFailed;
			sample.capabilities[index] = capability;
			if ((capability & 0xff) == 1) {
				if (pointer > 0xf8 || sample.pmOffset != 0)
					return InvalidCapabilities;
				sample.pmOffset = pointer;
			}
			pointer = capability >> 8;
		}
		if (sample.pmOffset == 0)
			return MissingPower;
		// A PM capability occupies eight bytes. Its PMCSR must not double as
		// another capability header, regardless of the traversal order.
		unsigned pmIndex = (sample.pmOffset - 0x40) / 4;
		if ((visited & (uint64_t(1) << (pmIndex + 1))) != 0)
			return InvalidCapabilities;
		uint32_t capabilities = 0, control = 0;
		if (!Read(read, context, sample.pmOffset + 2, 2, capabilities)
			|| !Read(read, context, sample.pmOffset + 4, 2, control))
			return ReadFailed;
		sample.pmCapabilities = capabilities;
		sample.pmControl = control;
		if ((control & 3) != 0)
			return NotD0;
		IntelN100PciSnapshot after = sample;
		result = Anchors(read, context, ownedBase, after);
		if (result != Success)
			return result;
		return Same(sample, after) ? Success : Unstable;
	}
};
#endif
