// SPDX-License-Identifier: MIT
// Inspect the real linked accelerant without initializing or calling hardware.
#include <OS.h>
#include <image.h>
#include <stdio.h>
#include <string.h>

#include "accelerant.h"

static unsigned sChecks;
static unsigned sFailures;

static void
Check(bool passed, const char* label, int model)
{
	++sChecks;
	if (!passed) {
		++sFailures;
		fprintf(stderr, "FAIL %s model=%#x\n", label, model);
	}
}

int
main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: hooks-test /path/to/intel_extreme.accelerant\n");
		return 2;
	}
	image_id image = load_add_on(argv[1]);
	if (image < B_OK) {
		fprintf(stderr, "load_add_on: %s\n", strerror(image));
		return 2;
	}
	void* symbol = NULL;
	if (get_image_symbol(image, B_ACCELERANT_ENTRY_POINT, B_SYMBOL_TYPE_TEXT,
			&symbol) != B_OK) {
		unload_add_on(image);
		return 2;
	}
	typedef void* (*get_hook)(uint32, void*);
	get_hook hook = reinterpret_cast<get_hook>(symbol);
	if (get_image_symbol(image, "gInfo", B_SYMBOL_TYPE_DATA, &symbol) != B_OK) {
		unload_add_on(image);
		return 2;
	}
	accelerant_info** infoSlot = static_cast<accelerant_info**>(symbol);
	accelerant_info* previous = *infoSlot;
	Check(previous == NULL, "addon starts uninitialized", 0);
	Check(hook(B_INIT_ACCELERANT, NULL) != NULL, "init lookup before init", 0);
	Check(hook(B_CLONE_ACCELERANT, NULL) != NULL, "clone lookup before init", 0);

	// GNU C++ permits designated aggregate initialization; DeviceType has
	// no default constructor. All omitted fields are value-initialized.
	intel_shared_info shared = {.device_type = 0};
	accelerant_info info = {};
	info.shared_info = &shared;
	*infoSlot = &info;

	struct test_case { int model; bool drawing; };
	const test_case cases[] = {
		{INTEL_MODEL_ALDM, false}, {INTEL_MODEL_TGLM, false},
		{INTEL_MODEL_SKY, false}, {INTEL_MODEL_KBY, false},
		{INTEL_MODEL_VLV, false}, {INTEL_MODEL_CHVM, false},
		{INTEL_MODEL_BDW, false},
		{INTEL_MODEL_915, true}, {INTEL_MODEL_945, true},
		{INTEL_MODEL_G45, true}, {INTEL_MODEL_SNBG, true},
		{INTEL_MODEL_IVBG, true}, {INTEL_MODEL_HAS, true}
	};
	const uint32 drawingHooks[] = {
		B_SCREEN_TO_SCREEN_BLIT, B_FILL_RECTANGLE, B_INVERT_RECTANGLE
	};
	const uint32 displayHooks[] = {
		B_GET_MODE_LIST, B_SET_DISPLAY_MODE, B_GET_DISPLAY_MODE,
		B_GET_FRAME_BUFFER_CONFIG, B_GET_PIXEL_CLOCK_LIMITS,
		B_GET_EDID_INFO, B_DPMS_CAPABILITIES, B_DPMS_MODE, B_SET_DPMS_MODE
	};
	for (const test_case& test : cases) {
		shared.device_type = test.model;
		for (uint32 feature : drawingHooks)
			Check((hook(feature, NULL) != NULL) == test.drawing,
				"drawing capability", test.model);
		for (uint32 feature : displayHooks)
			Check(hook(feature, NULL) != NULL, "display preserved", test.model);
		Check(hook(B_FILL_SPAN, NULL) == NULL, "span remains unavailable", test.model);
		Check(hook(0xffffffff, NULL) == NULL, "unknown hook", test.model);
	}
	*infoSlot = previous;
	unload_add_on(image);
	printf("%u checks, %u failures; hook lookup only, no GPU execution\n",
		sChecks, sFailures);
	return sFailures == 0 ? 0 : 1;
}
