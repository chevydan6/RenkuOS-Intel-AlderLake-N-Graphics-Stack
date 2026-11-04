// SPDX-License-Identifier: MIT
// Real ring setup/teardown, redirected to ordinary RAM. No GPU/device access.
#include <OS.h>
#include <image.h>
#include <stdio.h>
#include <string.h>

#include "accelerant.h"

static unsigned sChecks;
static unsigned sFailures;
static uint32 sRegisters[0x4000];

static void
check(bool pass, const char* label)
{
	sChecks++;
	if (!pass) {
		sFailures++;
		fprintf(stderr, "FAIL %s\n", label);
	}
}

int
main(int argc, char** argv)
{
	if (argc != 2)
		return 2;
	image_id image = load_add_on(argv[1]);
	if (image < B_OK)
		return 2;
	void* slot;
	void* entry;
	void* lifecycle;
	if (get_image_symbol(image, "gInfo", B_SYMBOL_TYPE_DATA, &slot) != B_OK
		|| get_image_symbol(image, B_ACCELERANT_ENTRY_POINT, B_SYMBOL_TYPE_TEXT,
			&entry) != B_OK
		|| get_image_symbol(image, "test_ring_lifecycle", B_SYMBOL_TYPE_TEXT,
			&lifecycle) != B_OK) {
		unload_add_on(image);
		return 2;
	}
	typedef void* (*get_hook)(uint32, void*);
	typedef uint32 (*engine_count)();
	typedef void (*run_lifecycle)(ring_buffer*);
	get_hook hook = reinterpret_cast<get_hook>(entry);
	run_lifecycle run = reinterpret_cast<run_lifecycle>(lifecycle);
	accelerant_info** infoSlot = static_cast<accelerant_info**>(slot);
	accelerant_info* previous = *infoSlot;
	intel_shared_info shared = {.device_type = INTEL_MODEL_ALDM};
	accelerant_info info = {};
	info.shared_info = &shared;
	info.registers = reinterpret_cast<uint8*>(sRegisters);
	*infoSlot = &info;
	engine_count count = reinterpret_cast<engine_count>(
		hook(B_ACCELERANT_ENGINE_COUNT, NULL));
	check(count != NULL && count() == 0, "Alder Lake engine count");
	const uint32 unavailable[] = {
		B_ACQUIRE_ENGINE, B_RELEASE_ENGINE, B_WAIT_ENGINE_IDLE,
		B_GET_SYNC_TOKEN, B_SYNC_TO_TOKEN, B_OVERLAY_COUNT,
		B_OVERLAY_SUPPORTED_SPACES, B_OVERLAY_SUPPORTED_FEATURES,
		B_ALLOCATE_OVERLAY_BUFFER, B_RELEASE_OVERLAY_BUFFER,
		B_GET_OVERLAY_CONSTRAINTS, B_ALLOCATE_OVERLAY, B_RELEASE_OVERLAY,
		B_CONFIGURE_OVERLAY
	};
	for (uint32 feature : unavailable)
		check(hook(feature, NULL) == NULL, "Alder Lake unsupported hook");
	ring_buffer ring = {};
	ring.lock.sem = -1;
	ring.register_base = INTEL_PRIMARY_RING_BUFFER;
	ring.size = 16 * B_PAGE_SIZE;
	ring.offset = 0x40000;
	ring_buffer saved;
	memcpy(&saved, &ring, sizeof(ring));
	run(&ring);
	check(memcmp(&ring, &saved, sizeof(ring)) == 0, "Alder Lake ring state");
	bool untouched = true;
	for (uint32 value : sRegisters)
		untouched &= value == 0;
	check(untouched, "Alder Lake register writes");

	// Positive control: the older supported path still creates/releases its
	// own semaphore and writes the simulated registers.
	shared.device_type = INTEL_MODEL_HAS;
	check(count != NULL && count() == 1, "legacy engine retained");
	check(hook(B_ACQUIRE_ENGINE, NULL) != NULL, "legacy engine hook retained");
	check(hook(B_OVERLAY_COUNT, NULL) != NULL, "legacy overlay count retained");
	memset(sRegisters, 0, sizeof(sRegisters));
	ring = saved;
	run(&ring);
	check(memcmp(&ring, &saved, sizeof(ring)) != 0, "legacy lifecycle exercised");
	bool changed = false;
	for (uint32 value : sRegisters)
		changed |= value != 0;
	check(changed, "legacy simulated register writes exercised");
	*infoSlot = previous;
	unload_add_on(image);
	printf("%u lifecycle checks, %u failures; simulated register RAM only\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
