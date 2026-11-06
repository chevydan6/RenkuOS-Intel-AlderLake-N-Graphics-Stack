/* SPDX-License-Identifier: MIT */
#include "AlderLakeHdmiFirmware.h"
#include <stdio.h>

static intel_shared_info Default() {
	intel_shared_info info = {};
	info.device_type.aldm = true; info.got_vbt = true;
	info.vbt_bdb_version = 251; info.vbt_child_device_size = sizeof(child_device_config);
	info.gpu_pci_vendor = 0x8086; info.gpu_pci_device = 0x46d1; info.gpu_pci_revision = 0;
	info.device_config_count = 1; info.pll_info.reference_frequency = 19200;
	info.device_configs[0].dvo_port = DVO_PORT_HDMIG;
	info.device_configs[0].device_type = 0x60d2;
	return info;
}
static status_t Read(intel_shared_info& info, bool layout,
	AlderLakeHdmiFirmware* out) {
	return ReadAlderLakeHdmiFirmware(&info, layout, INTEL_PORT_G, 148500,
		out);
}
int main() {
	unsigned errors = 0; AlderLakeHdmiFirmware out = {};
	intel_shared_info info = Default();
	if (Read(info, true, &out) != B_OK || out.tcPort != 2 || out.preset != 0
		|| out.pixelClockKHz != 148500 || out.referenceKHz != 19200
		|| out.laneReversal || out.loadgenWorkaround) errors++;
	if (Read(info, false, &out) != B_BAD_VALUE
		|| Read(info, true, NULL) != B_BAD_VALUE) errors++;
	info = Default(); info.gpu_pci_revision = 1;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.gpu_pci_device = 0x46d0;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_type.aldm = false;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.got_vbt = false;
	if (Read(info, true, &out) != B_OK || out.tcPort != 2) errors++;
	info = Default(); info.vbt_bdb_version = 183;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.vbt_child_device_size = offsetof(child_device_config, dvo_cfg) + 1;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_config_count = 11;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.pll_info.reference_frequency = 25000;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_configs[0].device_id[3] = 10;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_configs[0].device_type = 0;
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_config_count = 2; info.device_configs[1] = info.device_configs[0];
	if (Read(info, true, &out) != B_BAD_VALUE) errors++;
	info = Default(); info.device_configs[0].lane_reversal = true;
	if (Read(info, true, &out) != B_OK || !out.laneReversal) errors++;
	if (errors) { fprintf(stderr, "%u firmware cases failed\n", errors); return 1; }
	puts("12 Alder Lake HDMI firmware validation cases passed");
	return 0;
}
