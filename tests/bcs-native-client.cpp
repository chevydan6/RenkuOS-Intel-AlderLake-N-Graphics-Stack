// SPDX-License-Identifier: MIT
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "bcs-native-protocol.h"
int main()
{
	alarm(10); // Userland only; cannot interrupt a stuck MMIO load.
	int fd = open("/dev/misc/intel_bcs_inventory", O_RDONLY);
	if (fd < 0) { perror("open"); return 1; }
	BcsInventoryResult result = {};
	int status = ioctl(fd, BCS_INVENTORY_IOCTL, &result, sizeof(result));
	if (status < 0) perror("sample");
	close(fd);
	if (status < 0) return 1;
	const IntelN100Forcewake::Report& r = result.report;
	printf("version=%u module=%d transaction=%d error=%u cleanup=%u stage=%u ownership=%u\n",
		result.version, int(result.module), int(result.transaction), r.error, r.cleanup, r.stage, r.ownership);
	printf("ACK=%#x/%#x/%#x reads=%u writes=%u elapsed_us=%" PRIu64 "\n",
		r.initialAck, r.setAck, r.finalAck, r.reads, r.writes, r.elapsed);
	if (result.transaction == B_OK) {
		const IntelN100BcsInventory& s = result.registers;
		printf("tail[22030]=%#x head[22034]=%#x ctl[2203c]=%#x mode[2209c]=%#x msg_idle[800c]=%#x\n",
			s.tail, s.head, s.control, s.mode, s.pendingWake);
	}
	puts("Sequential raw BCS samples only; no engine-ready, reset, DMA or GPU execution claim.");
	return result.version == 1 && result.module == B_OK && result.transaction == B_OK
		&& r.error == 0 && r.cleanup == 0 && r.stage == IntelN100Forcewake::Complete
		&& r.ownership == IntelN100Forcewake::Released && r.writes == 2 && r.reads >= 5
		&& (r.initialAck & 0x8001) == 0 && (r.setAck & 0x8001) == 1
		&& (r.finalAck & 0x8001) == 0 && result.registers.tail != UINT32_MAX
		&& result.registers.head != UINT32_MAX && result.registers.control != UINT32_MAX
		&& result.registers.mode != UINT32_MAX && result.registers.pendingWake != UINT32_MAX ? 0 : 1;
}
