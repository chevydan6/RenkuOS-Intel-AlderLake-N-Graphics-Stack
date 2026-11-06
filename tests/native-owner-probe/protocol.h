// SPDX-License-Identifier: MIT
#ifndef TERRA_OWNER_PROBE_PROTOCOL_H
#define TERRA_OWNER_PROBE_PROTOCOL_H
#include <Drivers.h>
#include <OS.h>
#define TERRA_PROBE_GET (B_DEVICE_OP_CODES_END + 880)
#define TERRA_PROBE_CHECK (B_DEVICE_OP_CODES_END + 881)
#define TERRA_PROBE_SENTINEL 0x4a7b82c1u
#define TERRA_PROBE_USER_VALUE 0x5910ac42u
struct terra_probe_result {
	area_id area;
	status_t module_status;
	status_t begin_status;
	uint64 lease;
	uint32 outputs_unchanged;
};
#endif
